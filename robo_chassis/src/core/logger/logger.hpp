#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <fmt/core.h>
#include <memory>
#include <string>

namespace robo_chassis {

/**
 * @brief Уровни логирования (совместимы со старым API)
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief Класс для логирования с использованием spdlog
 * 
 * Обертка над spdlog для обратной совместимости со старым API.
 * Поддерживает ротацию файлов и вывод в консоль.
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
    
    // Удобные методы для разных уровней логирования (поддерживают fmt-style форматирование)
    template<typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        if (m_logger) m_logger->debug(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) {
        if (m_logger) m_logger->info(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(fmt::format_string<Args...> fmt, Args&&... args) {
        if (m_logger) m_logger->warn(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) {
        if (m_logger) m_logger->error(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void critical(fmt::format_string<Args...> fmt, Args&&... args) {
        if (m_logger) m_logger->critical(fmt, std::forward<Args>(args)...);
    }
    
    // Обратная совместимость со старым API (message, source)
    void debug(const std::string& msg, const std::string& src = "") { log(LogLevel::DEBUG, msg, src); }
    void info(const std::string& msg, const std::string& src = "") { log(LogLevel::INFO, msg, src); }
    void warning(const std::string& msg, const std::string& src = "") { log(LogLevel::WARNING, msg, src); }
    void error(const std::string& msg, const std::string& src = "") { log(LogLevel::ERROR, msg, src); }
    void critical(const std::string& msg, const std::string& src = "") { log(LogLevel::CRITICAL, msg, src); }
    
    /**
     * @brief Преобразовать уровень логирования в строку
     */
    static std::string levelToString(LogLevel level);
    
    /**
     * @brief Преобразовать строку в уровень логирования
     */
    static LogLevel stringToLevel(const std::string& str);
    
    /**
     * @brief Очистить зарегистрированный логгер (для тестов)
     */
    void shutdown();
    
private:
    Logger() = default;
    ~Logger() = default;
    
    // Запрет копирования
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::shared_ptr<spdlog::logger> m_logger;
    LogLevel min_level_{LogLevel::INFO};
};

} // namespace robo_chassis

// Макросы для удобного логирования с поддержкой форматирования (fmt-style)
#define LOG_DEBUG(fmt, ...) robo_chassis::Logger::instance().debug(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) robo_chassis::Logger::instance().info(fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) robo_chassis::Logger::instance().warning(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) robo_chassis::Logger::instance().error(fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) robo_chassis::Logger::instance().critical(fmt, ##__VA_ARGS__)

// Макросы с указанием источника (для обратной совместимости)
#define LOG_DEBUG_SRC(msg, src) robo_chassis::Logger::instance().debug("[{}] {}", src, msg)
#define LOG_INFO_SRC(msg, src) robo_chassis::Logger::instance().info("[{}] {}", src, msg)
#define LOG_WARNING_SRC(msg, src) robo_chassis::Logger::instance().warning("[{}] {}", src, msg)
#define LOG_ERROR_SRC(msg, src) robo_chassis::Logger::instance().error("[{}] {}", src, msg)
#define LOG_CRITICAL_SRC(msg, src) robo_chassis::Logger::instance().critical("[{}] {}", src, msg)
