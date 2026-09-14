#include <iostream>
#include <thread>
#include <vector>
#include <csignal>
#include <atomic>
#include <memory>
#include <chrono>
#include <unistd.h>   // Для write() - async-signal-safe функции
#include <cstring>    // Для strlen() - async-signal-safe функции

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

// ============================================================================
// Forward Declarations для функций инициализации
// ============================================================================

static void initLogger();
static bool initSafetyManager(RobotLogic& robot);
static bool initServers(RobotLogic& robot);
static bool initSensors(bool simulation_mode, RobotLogic& robot,
                       robo_chassis::SensorFusion& sensor_fusion,
                       robo_chassis::sensors::Ultrasonic& ultrasonic);
static bool initAutonomy(robo_chassis::AutonomyManager& autonomy,
                        bool simulation_mode,
                        robo_chassis::sensors::Compass* compass,
                        robo_chassis::sensors::Ultrasonic* ultrasonic,
                        robo_chassis::SensorFusion* fusion);
static int runMainLoop(RobotLogic& robot, 
                      robo_chassis::SensorFusion& sensor_fusion,
                      robo_chassis::sensors::Ultrasonic& ultrasonic,
                      robo_chassis::AutonomyManager& autonomy,
                      robo_chassis::SystemMonitor& sys_monitor,
                      bool simulation_mode);
static void cleanup();

// Helper функции для создания компонентов
static std::unique_ptr<robo_chassis::IExchanger> createI2CExchanger();
static robo_chassis::AutonomyManager createAutonomyManager(RobotLogic& robot);

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
        
        // Нормальный выход через установку флага вместо _exit
        // Основной цикл завершится и выполнит корректную очистку
        return;
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

// ============================================================================
// Функции инициализации и основного цикла
// ============================================================================

/**
 * @brief Инициализация логгера
 */
static void initLogger() {
    robo_chassis::Logger::instance().init(
        robo_chassis::LogLevel::INFO,
        true,   // console
        true,   // file
        "/var/log/robo_chassis/robot.log",
        10,     // max_size_mb
        5       // max_files
    );
}

/**
 * @brief Инициализация SafetyManager
 * @param robot Ссылка на RobotLogic для callback аварийной остановки
 * @return true при успешной инициализации
 */
static bool initSafetyManager(RobotLogic& robot) {
    g_safety_mgr = std::make_shared<robo_chassis::SafetyManager>(10, 5);  // 10 сек watchdog, 5 сек Arduino timeout
    
    if (!g_safety_mgr->init()) {
        LOG_ERROR("Не удалось инициализировать SafetyManager");
        return false;
    }
    
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
    return true;
}

/**
 * @brief Инициализация TCP и WebSocket серверов
 * @param robot Ссылка на RobotLogic для обработки команд
 * @return true при успешной инициализации
 */
static bool initServers(RobotLogic& robot) {
    const auto& tcp_config = robo_chassis::Config::getTcpServer();
    const auto& ws_config = robo_chassis::Config::getWebSocket();
    
    try {
        // TCP сервер для Python Bridge
        g_server = std::make_shared<TcpServer>(tcp_config.port, robot);
        LOG_INFO("TCP сервер запущен на порту " + std::to_string(tcp_config.port));
    } catch (const std::exception& e) {
        LOG_ERROR("Ошибка создания TCP сервера: " + std::string(e.what()));
        return false;
    }
    
    try {
        // WebSocket сервер для веб-интерфейса
        g_ws_server = std::make_shared<robo_chassis::WebSocketServer>(ws_config.port);
        
        // Установка callback для обработки команд
        g_ws_server->setCommandCallback([&robot](const Command& cmd) {
            robot.process_command(cmd);
        });
        
        // Обработчик команд автономности
        g_ws_server->setAutonomyCallback([](const std::string& payload) {
            try {
                json j = json::parse(payload);
                
                if (j.contains("auto_mode")) {
                    // Обработка будет выполнена в runMainLoop через AutonomyManager
                    LOG_DEBUG("Получена команда автономности: " + payload);
                }
            } catch (const json::parse_error& e) {
                LOG_WARNING("Ошибка парсинга JSON команды автономности: " + std::string(e.what()));
            } catch (const std::exception& e) {
                LOG_WARNING("Ошибка обработки команды автономности: " + std::string(e.what()));
            }
        });
        
        g_ws_server->start();
        LOG_INFO("WebSocket сервер запущен на порту " + std::to_string(ws_config.port));
    } catch (const std::exception& e) {
        LOG_ERROR("Ошибка создания WebSocket сервера: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

/**
 * @brief Инициализация датчиков (SensorFusion, Ultrasonic)
 * @param simulation_mode Режим симуляции
 * @param robot Ссылка на RobotLogic для инициализации IMU
 * @param sensor_fusion Ссылка на SensorFusion
 * @param ultrasonic Ссылка на Ultrasonic
 * @return true при успешной инициализации (или в режиме симуляции)
 */
static bool initSensors(bool simulation_mode, RobotLogic& robot,
                       robo_chassis::SensorFusion& sensor_fusion,
                       robo_chassis::sensors::Ultrasonic& ultrasonic) {
    const auto& i2c_config = robo_chassis::Config::getI2c();
    const auto& sensors_config = robo_chassis::Config::getSensors();
    
    if (simulation_mode) {
        LOG_INFO("Датчики отключены: режим симуляции активен");
        return true;
    }
    
    // Инициализация IMU
    if (i2c_config.imu_enabled) {
        robot.init_imu(i2c_config.device);
    } else {
        LOG_INFO("IMU отключен в конфигурации");
    }
    
    // Инициализация SensorFusion
    if (sensor_fusion.init()) {
        LOG_INFO("SensorFusion успешно инициализирован");
    } else {
        LOG_ERROR("Не удалось инициализировать SensorFusion");
    }
    
    // Инициализация ультразвукового дальномера
    ultrasonic.init();
    if (ultrasonic.isReady()) {
        ultrasonic.setMaxDistanceCm(sensors_config.ultrasonic_max_distance_cm);
        LOG_INFO("Ультразвуковой дальномер успешно инициализирован");
    } else {
        LOG_WARNING("Не удалось инициализировать ультразвуковой дальномер (проверьте GPIO)");
    }
    
    return true;
}

/**
 * @brief Инициализация AutonomyManager
 * @param autonomy Ссылка на AutonomyManager
 * @param simulation_mode Режим симуляции
 * @param compass Указатель на Compass (может быть nullptr)
 * @param ultrasonic Указатель на Ultrasonic (может быть nullptr)
 * @param fusion Указатель на SensorFusion (может быть nullptr)
 * @return true при успешной инициализации
 */
static bool initAutonomy(robo_chassis::AutonomyManager& autonomy,
                        bool simulation_mode,
                        robo_chassis::sensors::Compass* compass,
                        robo_chassis::sensors::Ultrasonic* ultrasonic,
                        robo_chassis::SensorFusion* fusion) {
    if (autonomy.init(compass, ultrasonic, fusion)) {
        LOG_INFO("AutonomyManager успешно инициализирован");
        return true;
    } else {
        LOG_WARNING("Не удалось инициализировать AutonomyManager");
        return false;
    }
}

/**
 * @brief Создание I2C обменника
 * @return unique_ptr к IExchanger
 */
static std::unique_ptr<robo_chassis::IExchanger> createI2CExchanger() {
    const auto& i2c_config = robo_chassis::Config::getI2c();
    
    std::unique_ptr<robo_chassis::IExchanger> i2c_exchanger;
    if (i2c_config.simulation_mode) {
        LOG_INFO("Запуск в режиме СИМУЛЯЦИИ (без реального Arduino)");
        i2c_exchanger = std::make_unique<robo_chassis::I2CSimulator>(9, 20);
    } else {
        LOG_INFO("Запуск в реальном режиме с подключением к Arduino по I2C");
        i2c_exchanger = std::make_unique<robo_chassis::I2CMaster>(
            i2c_config.device, 0x04, sizeof(ArduinoPkg), 20);
    }
    
    if (!i2c_exchanger->open()) {
        LOG_WARNING(i2c_config.simulation_mode ? 
                   "Не удалось запустить симулятор I2C" : 
                   "Не удалось открыть I2C соединение с Arduino");
    }
    
    return i2c_exchanger;
}

/**
 * @brief Создание AutonomyManager с callback для управления роботом
 * @param robot Ссылка на RobotLogic
 * @return AutonomyManager
 */
static robo_chassis::AutonomyManager createAutonomyManager(RobotLogic& robot) {
    return robo_chassis::AutonomyManager([&robot](const robo_chassis::ChassisCommand& cmd) {
        Command rc_cmd;
        rc_cmd.left_y = cmd.linear - cmd.angular * 0.5f;
        rc_cmd.right_y = cmd.linear + cmd.angular * 0.5f;
        rc_cmd.left_x = 0.0f;
        rc_cmd.right_x = 0.0f;
        rc_cmd.tower_h = 0;
        rc_cmd.fire = false;
        rc_cmd.lights = false;
        rc_cmd.pointer = false;
        
        // Ограничение значений [-1.0, 1.0]
        rc_cmd.left_y = std::clamp(rc_cmd.left_y, -1.0f, 1.0f);
        rc_cmd.right_y = std::clamp(rc_cmd.right_y, -1.0f, 1.0f);
        
        robot.process_command(rc_cmd);
    });
}

/**
 * @brief Основной цикл работы робота
 * @param robot Ссылка на RobotLogic
 * @param sensor_fusion Ссылка на SensorFusion
 * @param ultrasonic Ссылка на Ultrasonic
 * @param autonomy Ссылка на AutonomyManager
 * @param sys_monitor Ссылка на SystemMonitor
 * @param simulation_mode Режим симуляции
 * @return 0 при нормальном завершении, 1 при ошибке
 */
static int runMainLoop(RobotLogic& robot, 
                      robo_chassis::SensorFusion& sensor_fusion,
                      robo_chassis::sensors::Ultrasonic& ultrasonic,
                      robo_chassis::AutonomyManager& autonomy,
                      robo_chassis::SystemMonitor& sys_monitor,
                      bool simulation_mode) {
    const auto& i2c_config = robo_chassis::Config::getI2c();
    const auto& telemetry_config = robo_chassis::Config::getTelemetry();
    
    LOG_INFO("Запуск основного цикла...");
    LOG_INFO("Ожидание команд от Python Bridge на порту " + 
             std::to_string(robo_chassis::Config::getTcpServer().port));
    
    // Отправка первичной команды на Arduino для синхронизации
    if (robot.is_arduino_online()) {
        robot.send_to_arduino();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    int connection_attempts = 0;
    const int max_connection_attempts = telemetry_config.connection_timeout_attempts;
    auto last_sys_update = std::chrono::steady_clock::now();
    const auto sys_update_interval = std::chrono::milliseconds(500);
    
    // Rate limiting для телеметрии (2 Гц = 500мс)
    auto last_telemetry_send = std::chrono::steady_clock::now();
    const auto telemetry_interval = std::chrono::milliseconds(telemetry_config.update_interval_ms);
    
    while (g_running) {
        // Обновление автономного менеджера
        autonomy.update();
        
        // Сброс watchdog
        if (g_safety_mgr) {
            g_safety_mgr->petWatchdog();
            g_safety_mgr->updateArduinoStatus(robot.is_arduino_online());
        }
        
        // Отправка команд на Arduino и обновление телеметрии
        robot.send_to_arduino();
        robot.update_telemetry();
        
        // Периодическое обновление системного монитора
        auto now = std::chrono::steady_clock::now();
        if (now - last_sys_update >= sys_update_interval) {
            sys_monitor.update();
            MEM_UPDATE;
            last_sys_update = now;
        }
        
        // Отправка телеметрии через WebSocket с rate limiting
        if (now - last_telemetry_send >= telemetry_interval) {
            float heading = -1.0f;
            float distance_cm = 0.0f;
            
            if (!simulation_mode) {
                if (sensor_fusion.isInitialized()) {
                    sensor_fusion.update();
                    heading = sensor_fusion.getHeading();
                }
                if (ultrasonic.isReady()) {
                    distance_cm = ultrasonic.readDistanceCm();
                }
            } else {
                Telemetry telem_sim = robot.get_telemetry();
                heading = telem_sim.yaw;
                distance_cm = 50.0f;
            }
            
            Telemetry telem = robot.get_telemetry();
            g_ws_server->broadcastTelemetry(telem, 
                                       sys_monitor.getCpuTemperature(),
                                       sys_monitor.getMemoryUsagePercent(),
                                       heading,
                                       0.0f, 0.0f, 0.0f,
                                       distance_cm,
                                       sys_monitor.getWifiLinkQuality());
            
            last_telemetry_send = now;
            
            // Проверка на троттлинг
            if (sys_monitor.needsThrottling()) {
                std::string msg = "Системный троттлинг! Температура: " + 
                                 std::to_string(static_cast<int>(sys_monitor.getCpuTemperature())) + 
                                 "C, Память: " + 
                                 std::to_string(static_cast<int>(sys_monitor.getMemoryUsagePercent())) + "%";
                LOG_WARNING_SRC(msg, "main");
                
                auto recommendations = sys_monitor.getRecommendations();
                for (const auto& rec : recommendations) {
                    if (rec.find("CRITICAL") != std::string::npos || 
                        rec.find("WARNING") != std::string::npos) {
                        LOG_WARNING_SRC(rec, "main");
                    }
                }
            }
            
            // Автоматическая оптимизация памяти
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
        
        // Проверка подключения к Arduino
        if (!simulation_mode && !robot.is_arduino_online()) {
            connection_attempts++;
            if (connection_attempts >= max_connection_attempts) {
                LOG_WARNING("Arduino не отвечает в течение " + 
                           std::to_string(max_connection_attempts * 20) + " мс. Проверьте подключение.");
                connection_attempts = 0;
            }
        } else {
            connection_attempts = 0;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(telemetry_config.update_interval_ms));
        
        if (!g_running.load(std::memory_order_acquire)) {
            break;
        }
    }
    
    LOG_INFO("Выход из основного цикла. Завершение работы...");
    return 0;
}

/**
 * @brief Очистка ресурсов при завершении
 */
static void cleanup() {
    LOG_DEBUG("Остановка SafetyManager...");
    if (g_safety_mgr) {
        g_safety_mgr->stop();
        g_safety_mgr.reset();
    }
    
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
    
    // Сброс глобальных smart pointers
    g_gpio_controller.reset();
    
    LOG_INFO("Работа завершена корректно.");
}

// ============================================================================
// Main function - не более 100 строк
// ============================================================================

int main() {
    // Установка обработчика сигналов до инициализации компонентов
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGINT, &sa, nullptr) < 0 || sigaction(SIGTERM, &sa, nullptr) < 0) {
        std::cerr << "Ошибка установки обработчика сигналов" << std::endl;
        return 1;
    }
    
    try {
        // Инициализация логгера
        initLogger();
        LOG_INFO("=== RoboChassis Core (Pure C++20) ===");
        
        // Загрузка конфигурации
        if (!robo_chassis::Config::load("./config.json")) {
            LOG_WARNING("Не удалось загрузить конфигурацию, используются значения по умолчанию");
        }
        
        // Инициализация системного монитора и менеджера памяти
        robo_chassis::SystemMonitor sys_monitor;
        const auto& logging_config = robo_chassis::Config::getLogging();
        robo_chassis::MemoryManager::instance().init(
            logging_config.memory_cache_clear_threshold,
            logging_config.memory_critical_threshold
        );
        
        // Создание I2C обменника и RobotLogic
        auto i2c_exchanger = createI2CExchanger();
        RobotLogic robot(std::move(i2c_exchanger), robo_chassis::Config::getI2c().simulation_mode);
        
        // Инициализация датчиков
        robo_chassis::SensorFusion sensor_fusion;
        robo_chassis::sensors::Ultrasonic ultrasonic(
            robo_chassis::Config::getSensors().ultrasonic_trigger_pin,
            robo_chassis::Config::getSensors().ultrasonic_echo_pin
        );
        initSensors(robo_chassis::Config::getI2c().simulation_mode, robot, sensor_fusion, ultrasonic);
        
        // Инициализация серверов
        if (!initServers(robot)) {
            throw std::runtime_error("Ошибка инициализации серверов");
        }
        
        // Инициализация AutonomyManager и SafetyManager
        auto autonomy = createAutonomyManager(robot);
        initAutonomy(autonomy, robo_chassis::Config::getI2c().simulation_mode, 
                    nullptr, 
                    robo_chassis::Config::getI2c().simulation_mode ? nullptr : 
                        (ultrasonic.isReady() ? &ultrasonic : nullptr),
                    robo_chassis::Config::getI2c().simulation_mode ? nullptr :
                        (sensor_fusion.isInitialized() ? &sensor_fusion : nullptr));
        
        initSafetyManager(robot);
        
        // Запуск основного цикла
        int result = runMainLoop(robot, sensor_fusion, ultrasonic, autonomy, sys_monitor, 
                                robo_chassis::Config::getI2c().simulation_mode);
        
        // Очистка ресурсов
        cleanup();
        return result;
        
    } catch (const std::exception& e) {
        LOG_CRITICAL("Критическая ошибка: " + std::string(e.what()));
        if (g_safety_mgr) {
            g_safety_mgr->activateSafeMode();
            g_safety_mgr.reset();
        }
        g_server.reset();
        g_ws_server.reset();
        return 1;
    }
}
