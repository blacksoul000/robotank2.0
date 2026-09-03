#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <chrono>
#include <atomic>

namespace robo_chassis {

/**
 * @brief Уровни логирования
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief Класс для логирования с поддержкой ротации файлов
 * 
 * Реализует потокобезопасное логирование в консоль и файл.
 * Поддерживает ротацию логов по размеру файла.
 */
class Logger {
public:
    /**
     * @brief Получить единственный экземпляр логгера (Singleton)
     */
    static Logger& instance();
    
    /**
     * @brief Инициализация логгера
     * @param level Минимальный уровень логирования
     * @param enable_console Выводить ли логи в консоль
     * @param enable_file Записывать ли логи в файл
     * @param file_path Путь к файлу логов
     * @param max_size_mb Максимальный размер файла логов в МБ до ротации
     * @param max_files Максимальное количество файлов логов для хранения
     */
    void init(LogLevel level = LogLevel::INFO,
              bool enable_console = true,
              bool enable_file = true,
              const std::string& file_path = "/var/log/robo_chassis/robot.log",
              int max_size_mb = 10,
              int max_files = 5);
    
    /**
     * @brief Логирование сообщения
     * @param level Уровень логирования
     * @param message Сообщение
     * @param source Источник сообщения (имя файла или компонента)
     */
    void log(LogLevel level, const std::string& message, const std::string& source = "");
    
    // Удобные методы для разных уровней логирования
    void debug(const std::string& msg, const std::string& src = "") {
        log(LogLevel::DEBUG, msg, src);
    }
    
    void info(const std::string& msg, const std::string& src = "") {
        log(LogLevel::INFO, msg, src);
    }
    
    void warning(const std::string& msg, const std::string& src = "") {
        log(LogLevel::WARNING, msg, src);
    }
    
    void error(const std::string& msg, const std::string& src = "") {
        log(LogLevel::ERROR, msg, src);
    }
    
    void critical(const std::string& msg, const std::string& src = "") {
        log(LogLevel::CRITICAL, msg, src);
    }
    
    // Версии с поддержкой форматирования (printf-style)
    template<typename... Args>
    void debug(const std::string& format, const std::string& src, Args... args) {
        log_formatted(LogLevel::DEBUG, format, src, args...);
    }
    
    template<typename... Args>
    void info(const std::string& format, const std::string& src, Args... args) {
        log_formatted(LogLevel::INFO, format, src, args...);
    }
    
    template<typename... Args>
    void warning(const std::string& format, const std::string& src, Args... args) {
        log_formatted(LogLevel::WARNING, format, src, args...);
    }
    
    template<typename... Args>
    void error(const std::string& format, const std::string& src, Args... args) {
        log_formatted(LogLevel::ERROR, format, src, args...);
    }
    
    template<typename... Args>
    void critical(const std::string& format, const std::string& src, Args... args) {
        log_formatted(LogLevel::CRITICAL, format, src, args...);
    }
    
    /**
     * @brief Преобразовать уровень логирования в строку
     */
    static std::string levelToString(LogLevel level);
    
    /**
     * @brief Преобразовать строку в уровень логирования
     */
    static LogLevel stringToLevel(const std::string& str);
    
    // Метод для форматированного логирования (шаблонная реализация ниже)
    template<typename... Args>
    void log_formatted(LogLevel level, const std::string& format, const std::string& source, Args... args) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), format.c_str(), args...);
        log(level, std::string(buffer), source);
    }
    
private:
    Logger() = default;
    ~Logger();
    
    // Запрет копирования
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    /**
     * @brief Проверить размер файла и выполнить ротацию при необходимости
     */
    void checkRotation();
    
    /**
     * @brief Выполнить ротацию логов
     */
    void rotate();
    
    /**
     * @brief Получить текущую временную метку
     */
    std::string getTimestamp() const;
    
    std::mutex mutex_;
    std::ofstream file_stream_;
    
    std::atomic<bool> initialized_{false};
    std::atomic<bool> console_enabled_{true};
    std::atomic<bool> file_enabled_{false};
    
    LogLevel min_level_{LogLevel::INFO};
    std::string file_path_;
    int max_size_bytes_{10 * 1024 * 1024};  // 10 MB
    int max_files_{5};
};

} // namespace robo_chassis

// Макросы для удобного логирования с поддержкой форматирования (printf-style)
#define LOG_DEBUG(msg, ...) robo_chassis::Logger::instance().debug((msg), __FILE__, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) robo_chassis::Logger::instance().info((msg), __FILE__, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) robo_chassis::Logger::instance().warning((msg), __FILE__, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) robo_chassis::Logger::instance().error((msg), __FILE__, ##__VA_ARGS__)
#define LOG_CRITICAL(msg, ...) robo_chassis::Logger::instance().critical((msg), __FILE__, ##__VA_ARGS__)

// Макросы с указанием источника
#define LOG_DEBUG_SRC(msg, src) robo_chassis::Logger::instance().debug((msg), (src))
#define LOG_INFO_SRC(msg, src) robo_chassis::Logger::instance().info((msg), (src))
#define LOG_WARNING_SRC(msg, src) robo_chassis::Logger::instance().warning((msg), (src))
#define LOG_ERROR_SRC(msg, src) robo_chassis::Logger::instance().error((msg), (src))
#define LOG_CRITICAL_SRC(msg, src) robo_chassis::Logger::instance().critical((msg), (src))
