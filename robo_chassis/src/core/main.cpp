#include <iostream>
#include <thread>
#include <vector>
#include <csignal>
#include <atomic>
#include <memory>

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

std::atomic<bool> g_running{true};

void signal_handler(int signum) {
    LOG_INFO("Получен сигнал остановки (" + std::to_string(signum) + "). Завершение работы...");
    g_running = false;
}

int main() {
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
    
    // Установка обработчика сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

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
                sizeof(ArduinoPkg),
                20     // Интервал опроса 20 мс
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
            try {
                robot.init_imu(i2c_config.device);
                LOG_INFO("IMU успешно инициализирован");
            } catch (const std::exception& e) {
                LOG_ERROR("Ошибка инициализации IMU: " + std::string(e.what()));
            }
        } else {
            LOG_INFO("IMU отключен в конфигурации");
        }
        
        // 5. Запуск TCP сервера (для Python Bridge)
        TcpServer server(tcp_config.port, robot);
        
        // 6. Запуск WebSocket сервера (для веб-интерфейса)
        const auto& ws_config = robo_chassis::Config::getTcpServer(); // Используем тот же порт config
        robo_chassis::WebSocketServer ws_server(8765); // Порт WebSocket по умолчанию
        
        // 7. Инициализация SensorFusion (IMU + компас + ультразвук) - только в реальном режиме
        robo_chassis::SensorFusion sensor_fusion;
        if (i2c_config.simulation_mode) {
            LOG_INFO("SensorFusion отключен: режим симуляции активен");
        } else {
            try {
                if (sensor_fusion.init()) {
                    LOG_INFO("SensorFusion успешно инициализирован");
                } else {
                    LOG_ERROR("Не удалось инициализировать SensorFusion");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Ошибка инициализации SensorFusion: " + std::string(e.what()));
            }
        }
        
        // 8. Инициализация ультразвука отдельно - только в реальном режиме
        robo_chassis::sensors::Ultrasonic ultrasonic(17, 27); // GPIO 17 (Trigger), GPIO 27 (Echo)
        
        if (i2c_config.simulation_mode) {
            LOG_INFO("Ультразвуковой дальномер отключен: режим симуляции активен");
        } else {
            try {
                if (ultrasonic.init()) {
                    LOG_INFO("Ультразвуковой дальномер успешно инициализирован");
                    ultrasonic.setMaxDistanceCm(400.0f); // Максимальная дистанция 400 см
                } else {
                    LOG_WARNING("Не удалось инициализировать ультразвуковой дальномер (проверьте GPIO)");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Ошибка инициализации ультразвукового дальномера: " + std::string(e.what()));
            }
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

        // Запуск TCP сервера в отдельном потоке
        std::thread server_thread([&server]() {
            server.run();
        });
        
        // Запуск WebSocket сервера
        ws_server.setCommandCallback([&robot](const Command& cmd) {
            robot.process_command(cmd);
        });
        
        // Обработчик команд автономности
        ws_server.setAutonomyCallback([&autonomy](const std::string& payload) {
            // Парсинг JSON для извлечения режима и целевого курса
            auto findValue = [&](const std::string& key) -> std::string {
                size_t pos = payload.find("\"" + key + "\"");
                if (pos == std::string::npos) return "";
                pos = payload.find(':', pos);
                if (pos == std::string::npos) return "";
                pos++;
                while (pos < payload.size() && 
                       (payload[pos] == ' ' || payload[pos] == '\t')) pos++;
                if (pos >= payload.size()) return "";
                
                size_t end = pos;
                if (payload[pos] == '"') {
                    end = payload.find('"', pos + 1);
                    if (end == std::string::npos) return "";
                    return payload.substr(pos + 1, end - pos - 1);
                } else {
                    while (end < payload.size() && 
                           payload[end] != ',' && payload[end] != '}') {
                        end++;
                    }
                    return payload.substr(pos, end - pos);
                }
            };
            
            try {
                std::string mode = findValue("auto_mode");
                if (mode == "IDLE") {
                    autonomy.setState(robo_chassis::AutoState::IDLE);
                    LOG_INFO("Autonomy: IDLE mode activated");
                } else if (mode == "HOLD_HEADING") {
                    std::string heading_str = findValue("target_heading");
                    if (!heading_str.empty()) {
                        float heading = std::stof(heading_str);
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
            } catch (const std::exception& e) {
                LOG_WARNING("Ошибка парсинга команды автономности: " + std::string(e.what()));
            }
        });
        
        ws_server.start();

        // Отправка первичной команды на Arduino для синхронизации
        if (robot.is_arduino_online()) {
            robot.send_to_arduino();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Основной цикл обработки телеметрии и обмена с Arduino
        int connection_attempts = 0;
        const int max_connection_attempts = telemetry_config.connection_timeout_attempts;
        auto last_sys_update = std::chrono::steady_clock::now();
        const auto sys_update_interval = std::chrono::seconds(2); // Обновление каждые 2 секунды
        
        while (g_running) {
            // Обновление автономного менеджера (вызывается перед send_to_arduino)
            autonomy.update();
            
            // Отправка команд на Arduino
            robot.send_to_arduino();
            
            // Обновление телеметрии (чтение с Arduino и гироскопов)
            robot.update_telemetry();
            
            // Периодическое обновление системного монитора и менеджера памяти
            auto now = std::chrono::steady_clock::now();
            if (now - last_sys_update >= sys_update_interval) {
                sys_monitor.update();
                MEM_UPDATE;  // Обновление статистики памяти
                
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
                ws_server.broadcastTelemetry(telem, 
                                           sys_monitor.getCpuTemperature(),
                                           sys_monitor.getMemoryUsagePercent(),
                                           i2c_config.simulation_mode ? heading : (sensor_fusion.isInitialized() ? sensor_fusion.getHeading() : -1.0f),
                                           0.0f,  // mag_x не передается (используется внутри)
                                           0.0f,  // mag_y не передается (используется внутри)
                                           0.0f,  // mag_z не передается (используется внутри)
                                           i2c_config.simulation_mode ? distance_cm : (ultrasonic.isReady() ? ultrasonic.readDistanceCm() : -1.0f),
                                           sys_monitor.getWifiLinkQuality());
                
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
                    LOG_INFO("Выполнена автоматическая оптимизация памяти");
                }
                
                last_sys_update = now;
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
        }

        LOG_INFO("Остановка серверов...");
        ws_server.stop();
        server.stop();
        
        if (server_thread.joinable()) {
            server_thread.join();
        }

        LOG_INFO("Работа завершена.");

    } catch (const std::exception& e) {
        LOG_CRITICAL("Критическая ошибка: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
