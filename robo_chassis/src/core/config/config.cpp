#include "config.hpp"
#include "logger/logger.hpp"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace robo_chassis {

// Инициализация статического экземпляра
Config Config::instance_;

bool Config::load(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        LOG_WARNING("Configuration file not found: %s. Using defaults.", config_path.c_str());
        return false;
    }
    
    try {
        json j;
        file >> j;
        file.close();
        
        // Парсинг секции tcp_server
        if (j.contains("tcp_server")) {
            auto& tcp = j["tcp_server"];
            if (tcp.contains("port")) instance_.tcp_server_.port = tcp["port"].get<int>();
            if (tcp.contains("bind_address")) instance_.tcp_server_.bind_address = tcp["bind_address"].get<std::string>();
        }
        
        // Парсинг секции i2c
        if (j.contains("i2c")) {
            auto& i2c = j["i2c"];
            if (i2c.contains("device")) instance_.i2c_.device = i2c["device"].get<std::string>();
            if (i2c.contains("imu_enabled")) instance_.i2c_.imu_enabled = i2c["imu_enabled"].get<bool>();
            if (i2c.contains("simulation_mode")) instance_.i2c_.simulation_mode = i2c["simulation_mode"].get<bool>();
        }
        
        // Парсинг секции logging
        if (j.contains("logging")) {
            auto& logging = j["logging"];
            if (logging.contains("level")) instance_.logging_.level = logging["level"].get<std::string>();
            if (logging.contains("console")) instance_.logging_.console = logging["console"].get<bool>();
            if (logging.contains("file")) instance_.logging_.file = logging["file"].get<bool>();
            if (logging.contains("file_path")) instance_.logging_.file_path = logging["file_path"].get<std::string>();
            if (logging.contains("max_size_mb")) instance_.logging_.max_size_mb = logging["max_size_mb"].get<int>();
            if (logging.contains("max_files")) instance_.logging_.max_files = logging["max_files"].get<int>();
            if (logging.contains("memory_cache_clear_threshold")) 
                instance_.logging_.memory_cache_clear_threshold = logging["memory_cache_clear_threshold"].get<float>();
            if (logging.contains("memory_critical_threshold")) 
                instance_.logging_.memory_critical_threshold = logging["memory_critical_threshold"].get<float>();
        }
        
        // Парсинг секции telemetry
        if (j.contains("telemetry")) {
            auto& telemetry = j["telemetry"];
            if (telemetry.contains("update_interval_ms")) 
                instance_.telemetry_.update_interval_ms = telemetry["update_interval_ms"].get<int>();
            if (telemetry.contains("connection_timeout_attempts")) 
                instance_.telemetry_.connection_timeout_attempts = telemetry["connection_timeout_attempts"].get<int>();
        }
        
        // Парсинг секции watchdog
        if (j.contains("watchdog")) {
            auto& watchdog = j["watchdog"];
            if (watchdog.contains("enabled")) instance_.watchdog_.enabled = watchdog["enabled"].get<bool>();
            if (watchdog.contains("timeout_sec")) instance_.watchdog_.timeout_sec = watchdog["timeout_sec"].get<int>();
        }
        
        LOG_INFO("Configuration successfully loaded from %s", config_path.c_str());
        LOG_INFO("  TCP Server: port %d on %s", 
                 instance_.tcp_server_.port, instance_.tcp_server_.bind_address.c_str());
        LOG_INFO("  I2C: device %s%s", 
                 instance_.i2c_.device.c_str(),
                 instance_.i2c_.simulation_mode ? " (simulation mode)" : "");
        LOG_INFO("  Logging: level %s%s%s", 
                 instance_.logging_.level.c_str(),
                 instance_.logging_.file ? ", file: " : "",
                 instance_.logging_.file ? instance_.logging_.file_path.c_str() : "");
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Configuration parsing error: %s. Using defaults.", e.what());
        return false;
    }
}

} // namespace robo_chassis
