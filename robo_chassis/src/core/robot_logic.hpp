#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <functional>

// Структура телеметрии
struct Telemetry {
    float battery_voltage = 0.0f;
    float roll = 0.0f;      // Крен
    float pitch = 0.0f;     // Тангаж
    float turret_angle = 0.0f; // Угол башни
    int signal_quality = 100;
};

// Структура команды управления
struct Command {
    float left_x = 0.0f;
    float left_y = 0.0f;
    float right_x = 0.0f;
    float right_y = 0.0f;
    bool fire = false;
    bool lights = false;
};

class SerialPort;

class RobotLogic {
public:
    explicit RobotLogic(SerialPort& serial);
    
    // Обработка входящей команды
    void process_command(const Command& cmd);
    
    // Обновление телеметрии (чтение датчиков)
    void update_telemetry();
    
    // Получение текущей телеметрии
    Telemetry get_telemetry() const;
    
    // Проверка наличия новой телеметрии для отправки
    bool has_new_telemetry() const { return telemetry_updated; }
    void reset_telemetry_flag() { telemetry_updated = false; }

private:
    SerialPort& m_serial;
    Telemetry m_telemetry;
    Command m_current_cmd;
    std::atomic<bool> telemetry_updated{false};
    mutable std::mutex m_mutex;
    
    // Эмуляция чтения с MPU6050 и ADC (в реальности опрос I2C/UART)
    void read_sensors();
};
