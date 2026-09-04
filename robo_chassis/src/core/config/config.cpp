#include "config.hpp"
#include "logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace robo_chassis {

// Инициализация статического экземпляра
Config Config::instance_;

// Вспомогательная функция для поиска ключа в JSON
static std::string extractValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) {
        return "";
    }
    
    // Найти двоеточие после ключа
    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        return "";
    }
    
    // Пропустить пробелы
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
        pos++;
    }
    
    if (pos >= json.size()) {
        return "";
    }
    
    // Определить тип значения и извлечь его
    if (json[pos] == '"') {
        // Строка
        size_t end_pos = json.find('"', pos + 1);
        if (end_pos == std::string::npos) {
            return "";
        }
        return json.substr(pos + 1, end_pos - pos - 1);
    } else if (json[pos] == 't' || json[pos] == 'f') {
        // Boolean
        if (json.substr(pos, 4) == "true") {
            return "true";
        } else if (json.substr(pos, 5) == "false") {
            return "false";
        }
        return "";
    } else {
        // Число или другой тип
        size_t end_pos = pos;
        while (end_pos < json.size() && 
               (std::isdigit(json[end_pos]) || json[end_pos] == '.' || json[end_pos] == '-')) {
            end_pos++;
        }
        if (end_pos == pos) {
            return "";
        }
        return json.substr(pos, end_pos - pos);
    }
}

// Вспомогательная функция для извлечения вложенного объекта
static std::string extractObject(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) {
        return "";
    }
    
    // Найти открывающую скобку
    pos = json.find('{', pos);
    if (pos == std::string::npos) {
        return "";
    }
    
    // Найти соответствующую закрывающую скобку
    int brace_count = 1;
    size_t end_pos = pos + 1;
    while (end_pos < json.size() && brace_count > 0) {
        if (json[end_pos] == '{') {
            brace_count++;
        } else if (json[end_pos] == '}') {
            brace_count--;
        }
        end_pos++;
    }
    
    if (brace_count != 0) {
        return "";
    }
    
    return json.substr(pos, end_pos - pos);
}

bool Config::load(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "[Config] Файл конфигурации не найден: " << config_path 
                  << ". Используются значения по умолчанию.\n";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_content = buffer.str();
    file.close();
    
    return instance_.parseJson(json_content, config_path);
}

bool Config::parseJson(const std::string& json_content, const std::string& config_path) {
    try {
        // Парсинг секции tcp_server
        std::string tcp_obj = extractObject(json_content, "tcp_server");
        if (!tcp_obj.empty()) {
            std::string port = extractValue(tcp_obj, "port");
            std::string bind_addr = extractValue(tcp_obj, "bind_address");
            
            if (!port.empty()) instance_.tcp_server_.port = std::stoi(port);
            if (!bind_addr.empty()) instance_.tcp_server_.bind_address = bind_addr;
        }
        
        // Парсинг секции i2c
        std::string i2c_obj = extractObject(json_content, "i2c");
        if (!i2c_obj.empty()) {
            std::string device = extractValue(i2c_obj, "device");
            std::string imu_enabled = extractValue(i2c_obj, "imu_enabled");
            std::string simulation_mode = extractValue(i2c_obj, "simulation_mode");
            
            if (!device.empty()) instance_.i2c_.device = device;
            if (!imu_enabled.empty()) instance_.i2c_.imu_enabled = (imu_enabled == "true");
            if (!simulation_mode.empty()) instance_.i2c_.simulation_mode = (simulation_mode == "true");
        }
        
        // Парсинг секции logging
        std::string logging_obj = extractObject(json_content, "logging");
        if (!logging_obj.empty()) {
            std::string level = extractValue(logging_obj, "level");
            std::string console = extractValue(logging_obj, "console");
            std::string file = extractValue(logging_obj, "file");
            std::string file_path = extractValue(logging_obj, "file_path");
            std::string max_size = extractValue(logging_obj, "max_size_mb");
            std::string max_files = extractValue(logging_obj, "max_files");
            std::string mem_cache_thresh = extractValue(logging_obj, "memory_cache_clear_threshold");
            std::string mem_crit_thresh = extractValue(logging_obj, "memory_critical_threshold");
            
            if (!level.empty()) instance_.logging_.level = level;
            if (!console.empty()) instance_.logging_.console = (console == "true");
            if (!file.empty()) instance_.logging_.file = (file == "true");
            if (!file_path.empty()) instance_.logging_.file_path = file_path;
            if (!max_size.empty()) instance_.logging_.max_size_mb = std::stoi(max_size);
            if (!max_files.empty()) instance_.logging_.max_files = std::stoi(max_files);
            if (!mem_cache_thresh.empty()) instance_.logging_.memory_cache_clear_threshold = std::stof(mem_cache_thresh);
            if (!mem_crit_thresh.empty()) instance_.logging_.memory_critical_threshold = std::stof(mem_crit_thresh);
        }
        
        // Парсинг секции telemetry
        std::string telemetry_obj = extractObject(json_content, "telemetry");
        if (!telemetry_obj.empty()) {
            std::string interval = extractValue(telemetry_obj, "update_interval_ms");
            std::string timeout = extractValue(telemetry_obj, "connection_timeout_attempts");
            
            if (!interval.empty()) instance_.telemetry_.update_interval_ms = std::stoi(interval);
            if (!timeout.empty()) instance_.telemetry_.connection_timeout_attempts = std::stoi(timeout);
        }
        
        // Парсинг секции watchdog
        std::string watchdog_obj = extractObject(json_content, "watchdog");
        if (!watchdog_obj.empty()) {
            std::string enabled = extractValue(watchdog_obj, "enabled");
            std::string timeout = extractValue(watchdog_obj, "timeout_sec");
            
            if (!enabled.empty()) instance_.watchdog_.enabled = (enabled == "true");
            if (!timeout.empty()) instance_.watchdog_.timeout_sec = std::stoi(timeout);
        }
        
        std::cout << "[Config] Конфигурация успешно загружена из " << config_path << "\n";
        std::cout << "  TCP Server: порт " << instance_.tcp_server_.port 
                  << " на " << instance_.tcp_server_.bind_address << "\n";
        std::cout << "  I2C: устройство " << instance_.i2c_.device;
        if (instance_.i2c_.simulation_mode) {
            std::cout << " (режим симуляции)";
        }
        std::cout << "\n";
        std::cout << "  Logging: уровень " << instance_.logging_.level;
        if (instance_.logging_.file) {
            std::cout << ", файл: " << instance_.logging_.file_path;
        }
        std::cout << "\n";
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Config] Ошибка парсинга конфигурации: " << e.what() 
                  << ". Используются значения по умолчанию.\n";
        return false;
    }
}

} // namespace robo_chassis
