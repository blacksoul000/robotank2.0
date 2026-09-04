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
#include "serial_port.hpp"
#include "robot_logic.hpp"
#include "exchangers/uart_exchanger.hpp"
#include "websocket_server/websocket_server.hpp"
#include "sensors/compass_ultrasonic.hpp"

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
        const auto& serial_config = robo_chassis::Config::getSerial();
        const auto& tcp_config = robo_chassis::Config::getTcpServer();
        const auto& i2c_config = robo_chassis::Config::getI2c();
        const auto& telemetry_config = robo_chassis::Config::getTelemetry();
        
        // 1. Инициализация последовательного порта с повторными попытками
        SerialPort serial(serial_config.device, serial_config.baudrate, 
                         serial_config.max_retries, serial_config.retry_delay_ms);
        
        // 2. Создание UART обменника
        auto uart_exchanger = std::make_unique<robo_chassis::UartExchanger>(serial, sizeof(ArduinoPkg));
        
        // 3. Создание логики робота с использованием IExchanger
        RobotLogic robot(std::move(uart_exchanger));
        
        // 4. Инициализация IMU (гироскопы MPU6050)
        if (i2c_config.imu_enabled) {
            robot.init_imu(i2c_config.device);
        } else {
            LOG_INFO("IMU отключен в конфигурации");
        }
        
        // 5. Запуск TCP сервера (для Python Bridge)
        TcpServer server(tcp_config.port, robot);
        
        // 6. Запуск WebSocket сервера (для веб-интерфейса)
        const auto& ws_config = robo_chassis::Config::getTcpServer(); // Используем тот же порт config
        robo_chassis::WebSocketServer ws_server(8765); // Порт WebSocket по умолчанию
        
        // 7. Инициализация дополнительных датчиков (компас и ультразвук)
        robo_chassis::sensors::Compass compass(1, 0x1E); // I2C bus 1, адрес HMC5883L
        robo_chassis::sensors::Ultrasonic ultrasonic(17, 27); // GPIO 17 (Trigger), GPIO 27 (Echo)
        
        if (compass.init()) {
            LOG_INFO("Компас успешно инициализирован");
        } else {
            LOG_WARNING("Не удалось инициализировать компас (проверьте подключение I2C)");
        }
        
        if (ultrasonic.init()) {
            LOG_INFO("Ультразвуковой дальномер успешно инициализирован");
            ultrasonic.setMaxDistanceCm(400.0f); // Максимальная дистанция 400 см
        } else {
            LOG_WARNING("Не удалось инициализировать ультразвуковой дальномер (проверьте GPIO)");
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
        ws_server.start();

        // Открытие UART обменника для получения данных от Arduino
        if (serial.is_open()) {
            robot.send_to_arduino();  // Первичная отправка для синхронизации
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Основной цикл обработки телеметрии и обмена с Arduino
        int connection_attempts = 0;
        const int max_connection_attempts = telemetry_config.connection_timeout_attempts;
        auto last_sys_update = std::chrono::steady_clock::now();
        const auto sys_update_interval = std::chrono::seconds(2); // Обновление каждые 2 секунды
        
        while (g_running) {
            // Отправка команд на Arduino
            robot.send_to_arduino();
            
            // Обновление телеметрии (чтение с Arduino и гироскопов)
            robot.update_telemetry();
            
            // Периодическое обновление системного монитора и менеджера памяти
            auto now = std::chrono::steady_clock::now();
            if (now - last_sys_update >= sys_update_interval) {
                sys_monitor.update();
                MEM_UPDATE;  // Обновление статистики памяти
                
                // Чтение данных с дополнительных датчиков
                float heading = 0.0f;
                bool compass_ok = compass.isReady();
                if (compass_ok) {
                    heading = compass.getHeading();
                }
                
                float distance_cm = 0.0f;
                bool ultrasonic_ok = ultrasonic.isReady();
                if (ultrasonic_ok) {
                    distance_cm = ultrasonic.readDistanceCm();
                }
                
                // Отправка телеметрии через WebSocket (только heading, без mag_x/y/z)
                Telemetry telem = robot.get_telemetry();
                ws_server.broadcastTelemetry(telem, 
                                           sys_monitor.getCpuTemperature(),
                                           sys_monitor.getMemoryUsagePercent(),
                                           compass_ok ? heading : -1.0f,
                                           0.0f,  // mag_x не передается (используется внутри)
                                           0.0f,  // mag_y не передается (используется внутри)
                                           0.0f,  // mag_z не передается (используется внутри)
                                           ultrasonic_ok ? distance_cm : -1.0f,
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
            if (!robot.is_arduino_online()) {
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
