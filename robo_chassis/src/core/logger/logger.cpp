#include "logger/logger.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/null_sink.h>
#include <sys/stat.h>

namespace robo_chassis {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::init(LogLevel level, bool enable_console, bool enable_file,
                  const std::string& file_path, int max_size_mb, int max_files) {
    min_level_ = level;
    
    std::vector<spdlog::sink_ptr> sinks;
    
    // Консольный sink
    if (enable_console) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
        sinks.push_back(console_sink);
    }
    
    // Файловый sink с ротацией
    if (enable_file) {
        // Создать директорию для логов если она не существует
        size_t last_slash = file_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string dir_path = file_path.substr(0, last_slash);
            struct stat st;
            if (stat(dir_path.c_str(), &st) != 0) {
                std::string mkdir_cmd = "mkdir -p " + dir_path;
                system(mkdir_cmd.c_str());
            }
        }
        
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            file_path, 
            max_size_mb * 1024 * 1024,  // максимальный размер в байтах
            max_files                    // количество файлов
        );
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");
        sinks.push_back(file_sink);
    }
    
    if (sinks.empty()) {
        // Если ни один sink не включен, создаем null sink
        sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
    }
    
    auto logger = std::make_shared<spdlog::logger>("robo_chassis", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::level_enum(static_cast<int>(level)));
    logger->flush_on(spdlog::level::info);
    
    spdlog::register_logger(logger);
    m_logger = logger;
    
    LOG_INFO("Logger initialized: level={}, console={}, file={}{}", 
             levelToString(level),
             enable_console ? "yes" : "no",
             enable_file ? "yes" : "no",
             enable_file ? ", path: " + file_path : "");
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

LogLevel Logger::stringToLevel(const std::string& str) {
    if (str == "debug" || str == "DEBUG")    return LogLevel::DEBUG;
    if (str == "info" || str == "INFO")      return LogLevel::INFO;
    if (str == "warning" || str == "WARNING") return LogLevel::WARNING;
    if (str == "error" || str == "ERROR")    return LogLevel::ERROR;
    if (str == "critical" || str == "CRITICAL") return LogLevel::CRITICAL;
    return LogLevel::INFO;  // По умолчанию
}

void Logger::log(LogLevel level, const std::string& message, const std::string& source) {
    if (!m_logger || level < min_level_) {
        return;
    }
    
    if (!source.empty()) {
        size_t last_slash = source.find_last_of('/');
        std::string filename = (last_slash != std::string::npos) 
                               ? source.substr(last_slash + 1) 
                               : source;
        m_logger->log(spdlog::level::level_enum(static_cast<int>(level)), 
                     "[{}] {}", filename, message);
    } else {
        m_logger->log(spdlog::level::level_enum(static_cast<int>(level)), message);
    }
}

} // namespace robo_chassis
