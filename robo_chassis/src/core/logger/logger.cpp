#include "logger/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>

namespace robo_chassis {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::init(LogLevel level, bool enable_console, bool enable_file,
                  const std::string& file_path, int max_size_mb, int max_files) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    min_level_ = level;
    console_enabled_ = enable_console;
    file_enabled_ = enable_file;
    file_path_ = file_path;
    max_size_bytes_ = max_size_mb * 1024 * 1024;
    max_files_ = max_files;
    
    // Создать директорию для логов если она не существует
    if (file_enabled_) {
        size_t last_slash = file_path_.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string dir_path = file_path_.substr(0, last_slash);
            
            // Проверить существование директории
            struct stat st;
            if (stat(dir_path.c_str(), &st) != 0) {
                // Директория не существует, создать её
                std::string mkdir_cmd = "mkdir -p " + dir_path;
                system(mkdir_cmd.c_str());
            }
        }
        
        // Открыть файл для записи (добавление в конец)
        file_stream_.open(file_path_, std::ios::app);
        if (!file_stream_.is_open()) {
            std::cerr << "[Logger] Не удалось открыть файл для логирования: " 
                      << file_path_ << ". Логирование в файл отключено.\n";
            file_enabled_ = false;
        } else {
            checkRotation();
        }
    }
    
    initialized_ = true;
    
    std::cout << "[Logger] Инициализирован: уровень=" << levelToString(level)
              << ", консоль=" << (enable_console ? "да" : "нет")
              << ", файл=" << (enable_file ? "да" : "нет");
    if (enable_file) {
        std::cout << ", путь=" << file_path;
    }
    std::cout << "\n";
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
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

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Logger::checkRotation() {
    if (!file_stream_.is_open()) {
        return;
    }
    
    // Получить текущий размер файла
    file_stream_.seekp(0, std::ios::end);
    std::streampos current_size = file_stream_.tellp();
    
    if (current_size >= max_size_bytes_) {
        rotate();
    }
}

void Logger::rotate() {
    file_stream_.close();
    
    // Удалить самый старый файл
    std::string oldest_file = file_path_ + "." + std::to_string(max_files_);
    std::remove(oldest_file.c_str());
    
    // Переименовать существующие файлы
    for (int i = max_files_ - 1; i >= 1; --i) {
        std::string old_name = file_path_ + "." + std::to_string(i);
        std::string new_name = file_path_ + "." + std::to_string(i + 1);
        std::rename(old_name.c_str(), new_name.c_str());
    }
    
    // Переименовать текущий файл
    std::rename(file_path_.c_str(), (file_path_ + ".1").c_str());
    
    // Открыть новый файл
    file_stream_.open(file_path_, std::ios::app);
    if (!file_stream_.is_open()) {
        std::cerr << "[Logger] Ошибка при ротации: не удалось создать новый файл\n";
        file_enabled_ = false;
    }
}

void Logger::log(LogLevel level, const std::string& message, const std::string& source) {
    if (!initialized_ || level < min_level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string timestamp = getTimestamp();
    std::string level_str = levelToString(level);
    
    // Формировать сообщение
    std::stringstream ss;
    ss << "[" << timestamp << "] [" << level_str << "]";
    
    if (!source.empty()) {
        // Извлечь только имя файла из полного пути
        size_t last_slash = source.find_last_of('/');
        std::string filename = (last_slash != std::string::npos) 
                               ? source.substr(last_slash + 1) 
                               : source;
        ss << " [" << filename << "]";
    }
    
    ss << " " << message;
    
    std::string log_message = ss.str();
    
    // Вывод в консоль
    if (console_enabled_) {
        if (level == LogLevel::ERROR || level == LogLevel::CRITICAL) {
            std::cerr << log_message << std::endl;
        } else {
            std::cout << log_message << std::endl;
        }
    }
    
    // Запись в файл
    if (file_enabled_ && file_stream_.is_open()) {
        file_stream_ << log_message << std::endl;
        file_stream_.flush();
        
        // Проверить необходимость ротации
        checkRotation();
    }
}

} // namespace robo_chassis
