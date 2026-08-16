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
        
        // Парсинг секции throttling
        if (j.contains("throttling")) {
            auto& throttling = j["throttling"];
            if (throttling.contains("enabled")) instance_.throttling_.enabled = throttling["enabled"].get<bool>();
            if (throttling.contains("temperature_warning_threshold")) 
                instance_.throttling_.temperature_warning_threshold = throttling["temperature_warning_threshold"].get<float>();
            if (throttling.contains("temperature_critical_threshold")) 
                instance_.throttling_.temperature_critical_threshold = throttling["temperature_critical_threshold"].get<float>();
            if (throttling.contains("frequency_min_mhz")) 
                instance_.throttling_.frequency_min_mhz = throttling["frequency_min_mhz"].get<int>();
            if (throttling.contains("check_interval_ms")) 
                instance_.throttling_.check_interval_ms = throttling["check_interval_ms"].get<int>();
        }
        
        // Парсинг секции motors
        if (j.contains("motors")) {
            auto& motors = j["motors"];
            if (motors.contains("pwm_frequency")) instance_.motors_.pwm_frequency = motors["pwm_frequency"].get<int>();
            if (motors.contains("max_speed")) instance_.motors_.max_speed = motors["max_speed"].get<float>();
            if (motors.contains("acceleration_ramp")) instance_.motors_.acceleration_ramp = motors["acceleration_ramp"].get<float>();
            if (motors.contains("pid_kp")) instance_.motors_.pid_kp = motors["pid_kp"].get<float>();
            if (motors.contains("pid_ki")) instance_.motors_.pid_ki = motors["pid_ki"].get<float>();
            if (motors.contains("pid_kd")) instance_.motors_.pid_kd = motors["pid_kd"].get<float>();
        }
        
        // Парсинг секции sensors
        if (j.contains("sensors")) {
            auto& sensors = j["sensors"];
            if (sensors.contains("fusion_update_rate_hz")) 
                instance_.sensors_.fusion_update_rate_hz = sensors["fusion_update_rate_hz"].get<int>();
            
            // Настройки магнитометра
            if (sensors.contains("magnetometer_calib_min_x")) 
                instance_.sensors_.magnetometer_calib_min_x = sensors["magnetometer_calib_min_x"].get<float>();
            if (sensors.contains("magnetometer_calib_max_x")) 
                instance_.sensors_.magnetometer_calib_max_x = sensors["magnetometer_calib_max_x"].get<float>();
            if (sensors.contains("magnetometer_calib_min_y")) 
                instance_.sensors_.magnetometer_calib_min_y = sensors["magnetometer_calib_min_y"].get<float>();
            if (sensors.contains("magnetometer_calib_max_y")) 
                instance_.sensors_.magnetometer_calib_max_y = sensors["magnetometer_calib_max_y"].get<float>();
            if (sensors.contains("magnetometer_calib_min_z")) 
                instance_.sensors_.magnetometer_calib_min_z = sensors["magnetometer_calib_min_z"].get<float>();
            if (sensors.contains("magnetometer_calib_max_z")) 
                instance_.sensors_.magnetometer_calib_max_z = sensors["magnetometer_calib_max_z"].get<float>();
            
            // Настройки гироскопа
            if (sensors.contains("gyro_bias_x")) instance_.sensors_.gyro_bias_x = sensors["gyro_bias_x"].get<float>();
            if (sensors.contains("gyro_bias_y")) instance_.sensors_.gyro_bias_y = sensors["gyro_bias_y"].get<float>();
            if (sensors.contains("gyro_bias_z")) instance_.sensors_.gyro_bias_z = sensors["gyro_bias_z"].get<float>();
            
            // Настройки GPIO для ультразвукового дальномера
            if (sensors.contains("ultrasonic_trigger_pin"))
                instance_.sensors_.ultrasonic_trigger_pin = sensors["ultrasonic_trigger_pin"].get<int>();
            if (sensors.contains("ultrasonic_echo_pin"))
                instance_.sensors_.ultrasonic_echo_pin = sensors["ultrasonic_echo_pin"].get<int>();
            if (sensors.contains("ultrasonic_max_distance_cm"))
                instance_.sensors_.ultrasonic_max_distance_cm = sensors["ultrasonic_max_distance_cm"].get<float>();
        }
        
        // Парсинг секции websocket
        if (j.contains("websocket")) {
            auto& websocket = j["websocket"];
            if (websocket.contains("port")) instance_.websocket_.port = websocket["port"].get<int>();
            if (websocket.contains("bind_address")) instance_.websocket_.bind_address = websocket["bind_address"].get<std::string>();
            if (websocket.contains("max_clients")) instance_.websocket_.max_clients = websocket["max_clients"].get<int>();
            if (websocket.contains("rate_limit_messages_per_sec")) 
                instance_.websocket_.rate_limit_messages_per_sec = websocket["rate_limit_messages_per_sec"].get<int>();
            if (websocket.contains("compression_enabled")) 
                instance_.websocket_.compression_enabled = websocket["compression_enabled"].get<bool>();
        }
        
        // Парсинг секции safety
        if (j.contains("safety")) {
            auto& safety = j["safety"];
            if (safety.contains("emergency_stop_timeout_sec")) 
                instance_.safety_.emergency_stop_timeout_sec = safety["emergency_stop_timeout_sec"].get<float>();
            if (safety.contains("watchdog_hardware_enabled")) 
                instance_.safety_.watchdog_hardware_enabled = safety["watchdog_hardware_enabled"].get<bool>();
            if (safety.contains("watchdog_device")) 
                instance_.safety_.watchdog_device = safety["watchdog_device"].get<std::string>();
            if (safety.contains("watchdog_timeout_sec")) 
                instance_.safety_.watchdog_timeout_sec = safety["watchdog_timeout_sec"].get<int>();
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
