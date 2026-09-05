/**
 * @file test_config.cpp
 * @brief Тесты для модуля конфигурации
 * 
 * Проверяют загрузку, парсинг и доступ к настройкам конфигурации.
 */

#include <gtest/gtest.h>
#include "config/config.hpp"
#include <fstream>
#include <filesystem>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Сохраняем оригинальный конфиг если есть
        if (std::filesystem::exists("config.json")) {
            std::filesystem::rename("config.json", "config.json.bak");
        }
    }
    
    void TearDown() override {
        // Восстанавливаем оригинальный конфиг
        if (std::filesystem::exists("config.json.bak")) {
            std::filesystem::rename("config.json.bak", "config.json");
        }
        if (std::filesystem::exists("test_config.json")) {
            std::filesystem::remove("test_config.json");
        }
    }
    
    void CreateTestConfig(const std::string& content) {
        std::ofstream file("test_config.json");
        file << content;
    }
};

TEST_F(ConfigTest, DefaultConfiguration) {
    // Тест с конфигом по умолчанию (без файла)
    const auto& serial = robo_chassis::Config::getSerial();
    EXPECT_EQ(serial.device, "/dev/ttyUSB0");
    EXPECT_EQ(serial.baudrate, 115200);
    
    const auto& tcp = robo_chassis::Config::getTcpServer();
    EXPECT_EQ(tcp.port, 5555);
    EXPECT_EQ(tcp.bind_address, "0.0.0.0");
    
    const auto& logging = robo_chassis::Config::getLogging();
    EXPECT_EQ(logging.level, "info");
    EXPECT_TRUE(logging.console);
    EXPECT_TRUE(logging.file);
}

TEST_F(ConfigTest, CustomSerialConfig) {
    CreateTestConfig(R"({
        "serial": {
            "device": "/dev/ttyUSB1",
            "baudrate": 9600,
            "max_retries": 10
        }
    })");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_TRUE(loaded);
    
    const auto& serial = robo_chassis::Config::getSerial();
    EXPECT_EQ(serial.device, "/dev/ttyUSB1");
    EXPECT_EQ(serial.baudrate, 9600);
    EXPECT_EQ(serial.max_retries, 10);
}

TEST_F(ConfigTest, CustomTcpServerConfig) {
    CreateTestConfig(R"({
        "tcp_server": {
            "port": 8080,
            "bind_address": "127.0.0.1"
        }
    })");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_TRUE(loaded);
    
    const auto& tcp = robo_chassis::Config::getTcpServer();
    EXPECT_EQ(tcp.port, 8080);
    EXPECT_EQ(tcp.bind_address, "127.0.0.1");
}

TEST_F(ConfigTest, CustomLoggingConfig) {
    CreateTestConfig(R"({
        "logging": {
            "level": "debug",
            "console": false,
            "file": true,
            "file_path": "/tmp/test.log",
            "max_size_mb": 5,
            "max_files": 3
        }
    })");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_TRUE(loaded);
    
    const auto& logging = robo_chassis::Config::getLogging();
    EXPECT_EQ(logging.level, "debug");
    EXPECT_FALSE(logging.console);
    EXPECT_TRUE(logging.file);
    EXPECT_EQ(logging.file_path, "/tmp/test.log");
    EXPECT_EQ(logging.max_size_mb, 5);
    EXPECT_EQ(logging.max_files, 3);
}

TEST_F(ConfigTest, CustomMotorsConfig) {
    CreateTestConfig(R"({
        "motors": {
            "pwm_frequency": 2000,
            "max_speed": 0.8,
            "acceleration_ramp": 0.3,
            "pid_kp": 2.0,
            "pid_ki": 0.2,
            "pid_kd": 0.1
        }
    })");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_TRUE(loaded);
    
    const auto& motors = robo_chassis::Config::getMotors();
    EXPECT_EQ(motors.pwm_frequency, 2000);
    EXPECT_FLOAT_EQ(motors.max_speed, 0.8f);
    EXPECT_FLOAT_EQ(motors.acceleration_ramp, 0.3f);
    EXPECT_FLOAT_EQ(motors.pid_kp, 2.0f);
    EXPECT_FLOAT_EQ(motors.pid_ki, 0.2f);
    EXPECT_FLOAT_EQ(motors.pid_kd, 0.1f);
}

TEST_F(ConfigTest, CustomSensorsConfig) {
    CreateTestConfig(R"({
        "sensors": {
            "fusion_update_rate_hz": 100,
            "magnetometer_calib_min_x": -0.5,
            "magnetometer_calib_max_x": 0.5,
            "ultrasonic_trigger_pin": 18,
            "ultrasonic_echo_pin": 28,
            "ultrasonic_max_distance_cm": 500.0
        }
    })");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_TRUE(loaded);
    
    const auto& sensors = robo_chassis::Config::getSensors();
    EXPECT_EQ(sensors.fusion_update_rate_hz, 100);
    EXPECT_FLOAT_EQ(sensors.magnetometer_calib_min_x, -0.5f);
    EXPECT_FLOAT_EQ(sensors.magnetometer_calib_max_x, 0.5f);
    EXPECT_EQ(sensors.ultrasonic_trigger_pin, 18);
    EXPECT_EQ(sensors.ultrasonic_echo_pin, 28);
    EXPECT_FLOAT_EQ(sensors.ultrasonic_max_distance_cm, 500.0f);
}

TEST_F(ConfigTest, CustomWebSocketConfig) {
    CreateTestConfig(R"({
        "websocket": {
            "port": 9000,
            "bind_address": "0.0.0.0",
            "max_clients": 10,
            "rate_limit_messages_per_sec": 50,
            "compression_enabled": false
        }
    })");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_TRUE(loaded);
    
    const auto& ws = robo_chassis::Config::getWebSocket();
    EXPECT_EQ(ws.port, 9000);
    EXPECT_EQ(ws.max_clients, 10);
    EXPECT_EQ(ws.rate_limit_messages_per_sec, 50);
    EXPECT_FALSE(ws.compression_enabled);
}

TEST_F(ConfigTest, InvalidJsonFallbackToDefaults) {
    CreateTestConfig("{ invalid json }");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_FALSE(loaded);
    
    // Должны использоваться значения по умолчанию
    const auto& tcp = robo_chassis::Config::getTcpServer();
    EXPECT_EQ(tcp.port, 5555);
}

TEST_F(ConfigTest, EmptyJsonFallbackToDefaults) {
    CreateTestConfig("{}");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_TRUE(loaded);
    
    // Все значения должны быть по умолчанию
    const auto& serial = robo_chassis::Config::getSerial();
    EXPECT_EQ(serial.device, "/dev/ttyUSB0");
}

TEST_F(ConfigTest, NonExistentFileFallbackToDefaults) {
    bool loaded = robo_chassis::Config::load("non_existent_file.json");
    EXPECT_FALSE(loaded);
    
    const auto& tcp = robo_chassis::Config::getTcpServer();
    EXPECT_EQ(tcp.port, 5555);
}

TEST_F(ConfigTest, ConvenienceGetters) {
    CreateTestConfig(R"({
        "serial": {
            "device": "/dev/ttyUSB2",
            "baudrate": 57600
        },
        "tcp_server": {
            "port": 7777
        }
    })");
    
    bool loaded = robo_chassis::Config::load("test_config.json");
    EXPECT_TRUE(loaded);
    
    EXPECT_EQ(robo_chassis::Config::getSerialDevice(), "/dev/ttyUSB2");
    EXPECT_EQ(robo_chassis::Config::getSerialBaudrate(), 57600);
    EXPECT_EQ(robo_chassis::Config::getTcpPort(), 7777);
}
