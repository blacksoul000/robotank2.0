#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <functional>
#include <cstdint>
#include <array>
#include <memory>
#include <chrono>
#include <vector>

namespace robo_chassis {
    class IImu;
    class IExchanger;
}

// Структура телеметрии
struct Telemetry {
    float battery_voltage = 0.0f;
    float roll = 0.0f;      // Крен
    float pitch = 0.0f;     // Тангаж
    float yaw = 0.0f;       // Рыскание (угол башни относительно шасси)
    float turret_angle = 0.0f; // Абсолютный угол башни
    int16_t current_left = 0;
    int16_t current_right = 0;
    int16_t current_tower = 0;
    int signal_quality = 100;
    bool arduino_online = false;
    bool gyro_ready = false;
};

// Структура команды управления
struct Command {
    float left_x = 0.0f;
    float left_y = 0.0f;
    float right_x = 0.0f;
    float right_y = 0.0f;
    int16_t tower_h = 0;   // Положение башни по горизонтали
    bool fire = false;
    bool lights = false;
    bool pointer = false;  // Лазерный указатель
};

// Пакет данных для Arduino (от Raspberry Pi)
#pragma pack(push, 1)
struct RaspberryPkg {
    uint8_t powerDown : 1;
    uint8_t light : 1;
    uint8_t reserve : 6;
    int16_t leftEngine = 0;
    int16_t rightEngine = 0;
    int16_t towerH = 0;
    uint16_t crc = 0;
};
#pragma pack(pop)

// Пакет данных от Arduino (к Raspberry Pi)
#pragma pack(push, 1)
struct ArduinoPkg {
    int16_t voltage = 0;
    int16_t currentLeft;
    int16_t currentRight;
    int16_t currentTower;
    uint16_t crc = 0;
};
#pragma pack(pop)

class SerialPort;

class RobotLogic {
public:
    // Конструктор с SerialPort (для обратной совместимости)
    explicit RobotLogic(SerialPort& serial);
    
    // Конструктор с IExchanger (новый интерфейс)
    explicit RobotLogic(std::unique_ptr<robo_chassis::IExchanger> exchanger);
    
    ~RobotLogic();
    
    // Обработка входящей команды
    void process_command(const Command& cmd);
    
    // Обновление телеметрии (чтение датчиков и обмен с Arduino)
    void update_telemetry();
    
    // Отправка команд на Arduino
    void send_to_arduino();
    
    // Получение текущей телеметрии
    Telemetry get_telemetry() const;
    
    // Проверка наличия новой телеметрии для отправки
    bool has_new_telemetry() const { return telemetry_updated; }
    void reset_telemetry_flag() { telemetry_updated = false; }
    
    // Проверка онлайн статуса Arduino
    bool is_arduino_online() const { return m_telemetry.arduino_online; }
    
    // Калибровка гироскопа
    void calibrate_gyro();
    
    // Калибровка оружия (нулевой угол возвышения)
    void calibrate_gun();
    
    // Инициализация IMU
    void init_imu(const std::string& i2c_device = "/dev/i2c-1");

private:
    // Конструктор для внутренней логики
    RobotLogic();
    
    SerialPort* m_serial = nullptr;  // Для обратной совместимости
    std::unique_ptr<robo_chassis::IExchanger> m_exchanger;  // Новый интерфейс обмена
    Telemetry m_telemetry;
    Command m_current_cmd;
    RaspberryPkg m_out_package;
    ArduinoPkg m_offsets;  // Смещения для калибровки токов
    std::atomic<bool> telemetry_updated{false};
    mutable std::mutex m_mutex;
    
    // Таймаут для проверки онлайн статуса Arduino (мс)
    static constexpr int ARDUINO_TIMEOUT_MS = 1500;
    std::chrono::steady_clock::time_point m_last_arduino_response;
    
    // Состояние стрельбы
    bool m_shooting = false;
    bool m_shot_closing = false;
    
    // IMU устройства (шасси и башня)
    std::unique_ptr<robo_chassis::IImu> m_chassis_imu;
    std::unique_ptr<robo_chassis::IImu> m_tower_imu;
    float m_chassis_yaw_offset = 0.0f;
    float m_tower_yaw_offset = 0.0f;
    
    // CRC16 для проверки целостности пакетов
    uint16_t crc16(const unsigned char* data, unsigned short len) const;
    bool validate_arduino_package(const ArduinoPkg* pkg) const;
    
    // Чтение данных с MPU6050 (через I2C) и обработка телеметрии
    void read_sensors();
    void process_arduino_data(const uint8_t* data, size_t len);
    
    // Обработка данных от IExchanger
    void on_arduino_data_received(const std::vector<uint8_t>& data);
};
