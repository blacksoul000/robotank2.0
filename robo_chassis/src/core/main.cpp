#include <iostream>
#include <thread>
#include <vector>
#include <csignal>
#include <atomic>
#include <memory>
#include <chrono>
#include <unistd.h>  // Для write() - async-signal-safe функции

#include "config/config.hpp"
#include "logger/logger.hpp"
#include "system_monitor/system_monitor.hpp"
#include "memory/memory_manager.hpp"
#include "tcp_server.hpp"
#include "robot_logic.hpp"
#include "exchangers/i2c_master.hpp"
#include "exchangers/i2c_simulator.hpp"
#include "websocket_server/websocket_server.hpp"
#include "sensors/compass_ultrasonic.hpp"
#include "sensors/sensor_fusion.hpp"
#include "autonomy/autonomy_manager.hpp"
#include "safety/safety_manager.hpp"
#include "gpio/gpio_controller.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================================
// Signal Handler Variables - только async-signal-safe операции
// ============================================================================

// Флаг shutdown - используется в signal handler и основном цикле
// std::atomic является async-signal-safe для операций store/load
static std::atomic<bool> g_signal_shutdown_requested{false};

// Основной флаг работы приложения
std::atomic<bool> g_running{true};
std::atomic<bool> g_shutting_down{false};  // Флаг для предотвращения повторного входа
std::atomic<int> g_shutdown_stage{0};      // Стадия завершения для отладки

// Глобальные smart pointers для корректного управления временем жизни
// Инициализируются до регистрации обработчика сигналов (critical for safety)
static std::shared_ptr<robo_chassis::SafetyManager> g_safety_mgr;
static std::shared_ptr<robo_chassis::GpioController> g_gpio_controller;
static std::shared_ptr<TcpServer> g_server;
static std::shared_ptr<robo_chassis::WebSocketServer> g_ws_server;

/**
 * @brief Глобальный обработчик сигналов для graceful shutdown
 * 
 * ВАЖНО: В этом обработчике можно использовать ТОЛЬКО async-signal-safe функции!
 * Список async-signal-safe функций: https://man7.org/linux/man-pages/man7/signal-safety.7.html
 * 
 * Разрешено:
 * - std::atomic operations (store, load)
 * - write() для вывода в stderr/fd
 * 
 * ЗАПРЕЩЕНО:
 * - LOG_* макросы (используют malloc, mutex)
 * - printf, cout, cerr (не являются async-signal-safe)
 * - Вызов любых функций которые могут заблокироваться или выделить память
 */
static void signalHandler(int signum) {
    // Предотвращаем повторный вход в обработчик
    bool already_shutting_down = g_shutting_down.exchange(true);
    
    if (already_shutting_down) {
        // Если сигнал получен повторно во время shutdown - форсируем остановку
        // Используем только async-signal-safe операции
        
        // Устанавливаем флаг для основного цикла
        g_signal_shutdown_requested.store(true, std::memory_order_release);
        
        // Аварийная остановка двигателей если SafetyManager доступен
        if (g_safety_mgr) {
            g_safety_mgr->activateSafeMode();
        }
        
        // Вывод сообщения через async-signal-safe write()
        const char* msg = "\n⚠️  Повторный сигнал. Аварийная остановка.\n";
        write(STDERR_FILENO, msg, strlen(msg));
        
        // Немедленный выход без дополнительной очистки
        _exit(1);
    }
    
    // Первый сигнал - нормальный graceful shutdown
    g_shutdown_stage.store(1);
    g_signal_shutdown_requested.store(true, std::memory_order_release);
    g_running.store(false, std::memory_order_release);
    
    // Вывод сообщения через async-signal-safe write()
    const char* msg = "\n🛑 Получен сигнал. Завершение работы...\n";
    write(STDERR_FILENO, msg, strlen(msg));
    
    // Принудительная остановка двигателей через SafetyManager
    if (g_safety_mgr) {
        g_safety_mgr->activateSafeMode();
    }
    
    // Останавливаем серверы (они должны выйти из своих циклов)
    if (g_ws_server) {
        g_ws_server->stop();
    }
    if (g_server) {
        g_server->stop();
    }
    
    g_shutdown_stage.store(2);
}

int main() {
    // Установка обработчика сигналов до инициализации компонентов
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        std::cerr << "Ошибка установки обработчика SIGINT" << std::endl;
        return 1;
    }
    
    if (sigaction(SIGTERM, &sa, nullptr) < 0) {
        std::cerr << "Ошибка установки обработчика SIGTERM" << std::endl;
        return 1;
    }
    
    // Инициализация логгера первой (до использования макросов LOG_*)
    robo_chassis::Logger::instance().init(
        robo_chassis::LogLevel::INFO,
        true,   // console
        true,   // file
        "/var/log/robo_chassis/robot.log",
        10,     // max_size_mb
        5       // max_files
    );
    
    LOG_INFO("=== RoboChassis Core (Pure C++20) ===");
    
    // Загрузка конфигурации
    if (!robo_chassis::Config::load("./config.json")) {
        LOG_WARNING("Не удалось загрузить конфигурацию, используются значения по умолчанию");
    }
    
    try {
        // Централизованная инициализация компонентов
        LOG_INFO("Инициализация подсистем...");
        
        // Инициализация системного монитора
        robo_chassis::SystemMonitor sys_monitor;
        
        // Инициализация менеджера памяти
        const auto& logging_config = robo_chassis::Config::getLogging();
        robo_chassis::MemoryManager::instance().init(
            logging_config.memory_cache_clear_threshold,
            logging_config.memory_critical_threshold
        );
        
        // Получение настроек из конфигурации
        const auto& tcp_config = robo_chassis::Config::getTcpServer();
        const auto& i2c_config = robo_chassis::Config::getI2c();
        const auto& telemetry_config = robo_chassis::Config::getTelemetry();
        
        // 1. Создание I2C обменника (реальное устройство или симулятор)
        std::unique_ptr<robo_chassis::IExchanger> i2c_exchanger;
        
        if (i2c_config.simulation_mode) {
            // Режим симуляции - используем I2CSimulator
            LOG_INFO("Запуск в режиме СИМУЛЯЦИИ (без реального Arduino)");
            i2c_exchanger = std::make_unique<robo_chassis::I2CSimulator>(
                9,       // Размер пакета симулятора 9 байт
                20       // Интервал опроса 20 мс
            );
        } else {
            // Реальный режим - используем I2CMaster
            LOG_INFO("Запуск в реальном режиме с подключением к Arduino по I2C");
            i2c_exchanger = std::make_unique<robo_chassis::I2CMaster>(
                i2c_config.device, 
                0x04,  // Адрес Arduino по I2C
                sizeof(ArduinoPkg),
                20     // Интервал опроса 20 мс
            );
        }
        
        // 2. Открытие соединения
        if (!i2c_exchanger->open()) {
            if (i2c_config.simulation_mode) {
                LOG_WARNING("Не удалось запустить симулятор I2C");
            } else {
                LOG_WARNING("Не удалось открыть I2C соединение с Arduino");
            }
        }
        
        // 3. Создание логики робота с использованием IExchanger
        RobotLogic robot(std::move(i2c_exchanger), i2c_config.simulation_mode);
        
        // 4. Инициализация IMU (гироскопы MPU6050) - только в реальном режиме
        if (i2c_config.simulation_mode) {
            LOG_INFO("IMU отключен: режим симуляции активен");
        } else if (i2c_config.imu_enabled) {
            robot.init_imu(i2c_config.device);
        } else {
            LOG_INFO("IMU отключен в конфигурации");
        }
        
        // 5. Запуск TCP сервера (для Python Bridge)
        g_server = std::make_shared<TcpServer>(tcp_config.port, robot);
        
        // 6. Запуск WebSocket сервера (для веб-интерфейса)
        // Используем конфигурацию WebSocket из config.json
        const auto& ws_config = robo_chassis::Config::getWebSocket();
        g_ws_server = std::make_shared<robo_chassis::WebSocketServer>(ws_config.port);
        LOG_INFO("WebSocket сервер запущен на порту " + std::to_string(ws_config.port));
        
        // 7. Инициализация SensorFusion (IMU + компас + ультразвук) - только в реальном режиме
        robo_chassis::SensorFusion sensor_fusion;
        if (i2c_config.simulation_mode) {
            LOG_INFO("SensorFusion отключен: режим симуляции активен");
        } else if (sensor_fusion.init()) {
            LOG_INFO("SensorFusion успешно инициализирован");
        } else {
            LOG_ERROR("Не удалось инициализировать SensorFusion");
        }
        
        // 8. Инициализация ультразвука отдельно - только в реальном режиме
        const auto& sensors_config = robo_chassis::Config::getSensors();
        robo_chassis::sensors::Ultrasonic ultrasonic(
            sensors_config.ultrasonic_trigger_pin, 
            sensors_config.ultrasonic_echo_pin
        );
        
        if (i2c_config.simulation_mode) {
            LOG_INFO("Ультразвуковой дальномер отключен: режим симуляции активен");
        } else if (ultrasonic.init()) {
            LOG_INFO("Ультразвуковой дальномер успешно инициализирован");
            ultrasonic.setMaxDistanceCm(sensors_config.ultrasonic_max_distance_cm);
        } else {
            LOG_WARNING("Не удалось инициализировать ультразвуковой дальномер (проверьте GPIO)");
        }
        
        // 9. Инициализация AutonomyManager
        robo_chassis::AutonomyManager autonomy(
            [&robot](const robo_chassis::ChassisCommand& cmd) {
                // Преобразование команды автономности в команду для RobotLogic
                Command rc_cmd;
                rc_cmd.left_y = cmd.linear - cmd.angular * 0.5f;
                rc_cmd.right_y = cmd.linear + cmd.angular * 0.5f;
                rc_cmd.left_x = 0.0f;
                rc_cmd.right_x = 0.0f;
                rc_cmd.tower_h = 0;
                rc_cmd.fire = false;
                rc_cmd.lights = false;
                rc_cmd.pointer = false;
                
                // Ограничение значений в диапазоне [-1.0, 1.0]
                if (rc_cmd.left_y > 1.0f) rc_cmd.left_y = 1.0f;
                if (rc_cmd.left_y < -1.0f) rc_cmd.left_y = -1.0f;
                if (rc_cmd.right_y > 1.0f) rc_cmd.right_y = 1.0f;
                if (rc_cmd.right_y < -1.0f) rc_cmd.right_y = -1.0f;
                
                robot.process_command(rc_cmd);
            }
        );
        
        // Получение указателей на датчики для AutonomyManager
        robo_chassis::sensors::Compass* compass_ptr = nullptr;
        robo_chassis::sensors::Ultrasonic* ultrasonic_ptr = nullptr;
        robo_chassis::SensorFusion* fusion_ptr = nullptr;
        
        if (i2c_config.simulation_mode) {
            LOG_INFO("AutonomyManager работает с симулированными данными");
            // В режиме симуляции датчики не инициализируются, используются NULL-указатели
        } else {
            ultrasonic_ptr = ultrasonic.isReady() ? &ultrasonic : nullptr;
            fusion_ptr = sensor_fusion.isInitialized() ? &sensor_fusion : nullptr;
        }
        
        if (autonomy.init(compass_ptr, ultrasonic_ptr, fusion_ptr)) {
            LOG_INFO("AutonomyManager успешно инициализирован");
        } else {
            LOG_WARNING("Не удалось инициализировать AutonomyManager");
        }
        
        LOG_INFO("Запуск основного цикла...");
        LOG_INFO("Ожидание команд от Python Bridge на порту " + std::to_string(tcp_config.port));
        LOG_INFO("WebSocket сервер запущен на порту 8765");

        // Инициализация и запуск SafetyManager (watchdog + graceful shutdown)
        g_safety_mgr = std::make_shared<robo_chassis::SafetyManager>(10, 5);  // 10 сек watchdog, 5 сек Arduino timeout
        
        if (!g_safety_mgr->init()) {
            LOG_ERROR("Не удалось инициализировать SafetyManager");
        } else {
            // Установка обработчика аварийной остановки
            g_safety_mgr->setEmergencyStopCallback([&robot]() {
                Command safe_cmd;
                safe_cmd.left_y = 0.0f;
                safe_cmd.right_y = 0.0f;
                safe_cmd.left_x = 0.0f;
                safe_cmd.right_x = 0.0f;
                safe_cmd.tower_h = 0;
                safe_cmd.fire = false;
                safe_cmd.lights = false;
                safe_cmd.pointer = false;
                robot.process_command(safe_cmd);
            });
            
            g_safety_mgr->start();
            LOG_INFO("SafetyManager запущен");
        }
        
        // Запуск TCP сервера в отдельном потоке
        std::thread server_thread([server = g_server]() {
            server->run();
        });
        
        // Запуск WebSocket сервера
        g_ws_server->setCommandCallback([&robot](const Command& cmd) {
            robot.process_command(cmd);
        });
        
        // Обработчик команд автономности с использованием nlohmann/json для безопасного парсинга
        g_ws_server->setAutonomyCallback([&autonomy](const std::string& payload) {
            try {
                // Используем безопасный парсер nlohmann/json
                json j = json::parse(payload);
                
                if (j.contains("auto_mode")) {
                    std::string mode = j["auto_mode"].get<std::string>();
                    
                    if (mode == "IDLE") {
                        autonomy.setState(robo_chassis::AutoState::IDLE);
                        LOG_INFO("Autonomy: IDLE mode activated");
                    } else if (mode == "HOLD_HEADING") {
                        if (j.contains("target_heading")) {
                            float heading = j["target_heading"].get<float>();
                            autonomy.setTargetHeading(heading);
                            autonomy.setState(robo_chassis::AutoState::HOLD_HEADING);
                            LOG_INFO("Autonomy: HOLD_HEADING mode, target=" + std::to_string(heading));
                        }
                    } else if (mode == "AVOID_OBSTACLE") {
                        autonomy.setState(robo_chassis::AutoState::AVOID_OBSTACLE);
                        LOG_INFO("Autonomy: AVOID_OBSTACLE mode activated");
                    } else if (mode == "PATROL") {
                        autonomy.setState(robo_chassis::AutoState::PATROL);
                        LOG_INFO("Autonomy: PATROL mode activated");
                    }
                }
            } catch (const json::parse_error& e) {
                LOG_WARNING("Ошибка парсинга JSON команды автономности: " + std::string(e.what()));
            } catch (const std::exception& e) {
                LOG_WARNING("Ошибка обработки команды автономности: " + std::string(e.what()));
            }
        });
        
        g_ws_server->start();

        // Отправка первичной команды на Arduino для синхронизации
        if (robot.is_arduino_online()) {
            robot.send_to_arduino();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Основной цикл обработки телеметрии и обмена с Arduino
        int connection_attempts = 0;
        const int max_connection_attempts = telemetry_config.connection_timeout_attempts;
        auto last_sys_update = std::chrono::steady_clock::now();
        const auto sys_update_interval = std::chrono::milliseconds(500); // Обновление системы каждые 500мс
        
        // Rate limiting для телеметрии (2 Гц = 500мс)
        auto last_telemetry_send = std::chrono::steady_clock::now();
        const auto telemetry_interval = std::chrono::milliseconds(500); // 2 Hz
        
        while (g_running) {
            // Обновление автономного менеджера (вызывается перед send_to_arduino)
            autonomy.update();
            
            // Сброс watchdog (pet) в основном цикле
            if (g_safety_mgr) {
                g_safety_mgr->petWatchdog();
            }
            
            // Обновление статуса Arduino для SafetyManager
            if (g_safety_mgr) {
                g_safety_mgr->updateArduinoStatus(robot.is_arduino_online());
            }
            
            // Отправка команд на Arduino
            robot.send_to_arduino();
            
            // Обновление телеметрии (чтение с Arduino и гироскопов)
            robot.update_telemetry();
            
            // Периодическое обновление системного монитора и менеджера памяти
            auto now = std::chrono::steady_clock::now();
            if (now - last_sys_update >= sys_update_interval) {
                sys_monitor.update();
                MEM_UPDATE;  // Обновление статистики памяти
                last_sys_update = now;
            }
            
            // Отправка телеметрии через WebSocket с rate limiting (2 Гц)
            if (now - last_telemetry_send >= telemetry_interval) {
                // Чтение данных из SensorFusion и ультразвука - только в реальном режиме
                float heading = -1.0f;
                float distance_cm = 0.0f;
                
                if (!i2c_config.simulation_mode) {
                    bool fusion_ready = sensor_fusion.isInitialized();
                    if (fusion_ready) {
                        sensor_fusion.update();
                        heading = sensor_fusion.getHeading();
                    }
                    
                    bool ultrasonic_ok = ultrasonic.isReady();
                    if (ultrasonic_ok) {
                        distance_cm = ultrasonic.readDistanceCm();
                    }
                } else {
                    // В режиме симуляции получаем данные от I2CSimulator
                    Telemetry telem_sim = robot.get_telemetry();
                    heading = telem_sim.yaw;  // Используем yaw от симулятора
                    distance_cm = 50.0f;      // Фиксированное значение для симуляции
                }
                
                // Отправка телеметрии через WebSocket
                Telemetry telem = robot.get_telemetry();
                g_ws_server->broadcastTelemetry(telem, 
                                           sys_monitor.getCpuTemperature(),
                                           sys_monitor.getMemoryUsagePercent(),
                                           i2c_config.simulation_mode ? heading : (sensor_fusion.isInitialized() ? sensor_fusion.getHeading() : -1.0f),
                                           0.0f,  // mag_x не передается (используется внутри)
                                           0.0f,  // mag_y не передается (используется внутри)
                                           0.0f,  // mag_z не передается (используется внутри)
                                           i2c_config.simulation_mode ? distance_cm : (ultrasonic.isReady() ? ultrasonic.readDistanceCm() : -1.0f),
                                           sys_monitor.getWifiLinkQuality());
                
                last_telemetry_send = now;
                
                // Проверка на троттлинг и критические состояния
                if (sys_monitor.needsThrottling()) {
                    std::string msg = "Системный троттлинг! Температура: " + 
                                     std::to_string(static_cast<int>(sys_monitor.getCpuTemperature())) + 
                                     "C, Память: " + 
                                     std::to_string(static_cast<int>(sys_monitor.getMemoryUsagePercent())) + "%";
                    LOG_WARNING_SRC(msg, "main");
                    
                    // Рекомендации по оптимизации
                    auto recommendations = sys_monitor.getRecommendations();
                    for (const auto& rec : recommendations) {
                        if (rec.find("CRITICAL") != std::string::npos || 
                            rec.find("WARNING") != std::string::npos) {
                            LOG_WARNING_SRC(rec, "main");
                        }
                    }
                }
                
                // Автоматическая оптимизация памяти при необходимости
                if (MEM_OPTIMIZE) {
                    LOG_DEBUG("Выполнена автоматическая оптимизация памяти");
                }
            }
            
            // Вывод телеметрии для отладки
            if (robot.has_new_telemetry()) {
                Telemetry t = robot.get_telemetry();
                LOG_DEBUG_SRC("Bat: " + std::to_string(t.battery_voltage) + "V, " +
                             "Roll: " + std::to_string(t.roll) + ", Pitch: " + std::to_string(t.pitch) +
                             ", Yaw: " + std::to_string(t.yaw) + ", Turret: " + std::to_string(t.turret_angle) +
                             " | L: " + std::to_string(t.current_left) + "mA, R: " + std::to_string(t.current_right) +
                             "mA, T: " + std::to_string(t.current_tower) + "mA" +
                             " | Arduino: " + std::string(t.arduino_online ? "ON" : "OFF") +
                             ", Gyro: " + std::string(t.gyro_ready ? "READY" : "INIT"),
                             "telemetry");
                robot.reset_telemetry_flag();
            }
            
            // Проверка статуса подключения к Arduino и попытки переподключения
            // (пропускаем в режиме симуляции)
            if (!i2c_config.simulation_mode && !robot.is_arduino_online()) {
                connection_attempts++;
                if (connection_attempts >= max_connection_attempts) {
                    LOG_WARNING("Arduino не отвечает в течение " + 
                               std::to_string(max_connection_attempts * 20) + " мс. Проверьте подключение.");
                    connection_attempts = 0;  // Сброс для повторных предупреждений
                }
            } else {
                connection_attempts = 0;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(telemetry_config.update_interval_ms));
            
            // Дополнительная проверка флага остановки после sleep
            if (!g_running.load(std::memory_order_acquire)) {
                break;
            }
        }

        LOG_INFO("Выход из основного цикла. Завершение работы...");
        
        // Остановка SafetyManager ДО серверов (чтобы watchdog не сработал во время shutdown)
        LOG_DEBUG("Остановка SafetyManager...");
        if (g_safety_mgr) {
            g_safety_mgr->stop();
            g_safety_mgr.reset();
        }
        
        // Остановка серверов
        LOG_DEBUG("Остановка WebSocket сервера...");
        if (g_ws_server) {
            g_ws_server->stop();
            g_ws_server.reset();
        }
        
        LOG_DEBUG("Остановка TCP сервера...");
        if (g_server) {
            g_server->stop();
            g_server.reset();
        }
        
        // Join потока TCP сервера с таймаутом
        if (server_thread.joinable()) {
            LOG_DEBUG("Ожидание завершения потока TCP сервера...");
            server_thread.join();
        }

        LOG_INFO("Работа завершена корректно.");

    } catch (const std::exception& e) {
        LOG_CRITICAL("Критическая ошибка: " + std::string(e.what()));
        
        // Аварийная остановка при исключении
        if (g_safety_mgr) {
            g_safety_mgr->activateSafeMode();
            g_safety_mgr.reset();
        }
        g_server.reset();
        g_ws_server.reset();
        
        return 1;
    }
    
    // Сброс глобальных smart pointers перед выходом
    g_safety_mgr.reset();
    g_gpio_controller.reset();
    g_server.reset();
    g_ws_server.reset();

    return 0;
}
