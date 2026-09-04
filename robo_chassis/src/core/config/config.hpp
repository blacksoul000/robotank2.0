#pragma once

#include <string>
#include <cstdint>

namespace robo_chassis {

/**
 * @brief Структура для хранения настроек последовательного порта
 */
struct SerialConfig {
    std::string device = "/dev/ttyUSB0";
    int baudrate = 115200;
    int max_retries = 5;
    int retry_delay_ms = 1000;
};

/**
 * @brief Структура для хранения настроек TCP сервера
 */
struct TcpServerConfig {
    int port = 5555;
    std::string bind_address = "0.0.0.0";
};

/**
 * @brief Структура для хранения I2C настроек
 */
struct I2cConfig {
    std::string device = "/dev/i2c-1";
    bool imu_enabled = true;
    bool simulation_mode = false;
};

/**
 * @brief Структура для хранения настроек логирования
 */
struct LoggingConfig {
    std::string level = "info";  // debug, info, warning, error, critical
    bool console = true;
    bool file = true;
    std::string file_path = "/var/log/robo_chassis/robot.log";
    int max_size_mb = 10;
    int max_files = 5;
    float memory_cache_clear_threshold = 0.85f;  // Порог очистки кэша (85%)
    float memory_critical_threshold = 0.95f;     // Критический порог (95%)
};

/**
 * @brief Структура для хранения настроек телеметрии
 */
struct TelemetryConfig {
    int update_interval_ms = 50;  // Увеличено с 20 до 50 мс для снижения нагрузки на RPi 2B
    int connection_timeout_attempts = 10;
};

/**
 * @brief Структура для хранения настроек watchdog
 */
struct WatchdogConfig {
    bool enabled = false;
    int timeout_sec = 30;
};

/**
 * @brief Класс для загрузки и управления конфигурацией приложения
 * 
 * Загружает настройки из JSON файла config.json в корне проекта.
 * Если файл не найден или содержит ошибки, используются значения по умолчанию.
 */
class Config {
public:
    /**
     * @brief Загрузка конфигурации из файла
     * @param config_path Путь к файлу конфигурации (по умолчанию "./config.json")
     * @return true если конфигурация успешно загружена, false если использованы значения по умолчанию
     */
    static bool load(const std::string& config_path = "./config.json");
    
    // Геттеры для всех секций конфигурации
    static const SerialConfig& getSerial() { return instance_.serial_; }
    static const TcpServerConfig& getTcpServer() { return instance_.tcp_server_; }
    static const I2cConfig& getI2c() { return instance_.i2c_; }
    static const LoggingConfig& getLogging() { return instance_.logging_; }
    static const TelemetryConfig& getTelemetry() { return instance_.telemetry_; }
    static const WatchdogConfig& getWatchdog() { return instance_.watchdog_; }
    
    // Удобные геттеры для часто используемых значений
    static std::string getSerialDevice() { return instance_.serial_.device; }
    static int getSerialBaudrate() { return instance_.serial_.baudrate; }
    static int getTcpPort() { return instance_.tcp_server_.port; }
    static std::string getI2cDevice() { return instance_.i2c_.device; }
    static bool isImuEnabled() { return instance_.i2c_.imu_enabled; }
    static bool isSimulationMode() { return instance_.i2c_.simulation_mode; }
    
private:
    static Config instance_;
    
    SerialConfig serial_;
    TcpServerConfig tcp_server_;
    I2cConfig i2c_;
    LoggingConfig logging_;
    TelemetryConfig telemetry_;
    WatchdogConfig watchdog_;
    
    // Приватный конструктор для паттерна Singleton
    Config() = default;
    
    // Парсинг JSON (упрощенная реализация без внешних зависимостей)
    bool parseJson(const std::string& json_content, const std::string& config_path);
};

} // namespace robo_chassis
