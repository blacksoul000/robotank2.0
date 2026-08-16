#include "robot_logic.hpp"
#include "serial_port.hpp"
#include <iostream>
#include <chrono>
#include <random>

RobotLogic::RobotLogic(SerialPort& serial) : m_serial(serial) {
    std::cout << "RobotLogic инициализирован.\n";
}

void RobotLogic::process_command(const Command& cmd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_current_cmd = cmd;
    
    // Формирование пакета для Arduino
    // Формат: 'M', left_y, left_x, right_y, right_x, flags
    uint8_t flags = 0;
    if (cmd.fire) flags |= 0x01;
    if (cmd.lights) flags |= 0x02;
    
    uint8_t buffer[7];
    buffer[0] = 'M'; // Маркер начала пакета моторов
    buffer[1] = static_cast<uint8_t>((cmd.left_y + 1.0f) * 127.0f);   // 0-255
    buffer[2] = static_cast<uint8_t>((cmd.left_x + 1.0f) * 127.0f);
    buffer[3] = static_cast<uint8_t>((cmd.right_y + 1.0f) * 127.0f);
    buffer[4] = static_cast<uint8_t>((cmd.right_x + 1.0f) * 127.0f);
    buffer[5] = flags;
    buffer[6] = '\n';
    
    m_serial.write(buffer, 7);
    
    if (cmd.fire) {
        std::cout << "ОГОНЬ!\n";
    }
}

void RobotLogic::update_telemetry() {
    read_sensors();
    telemetry_updated = true;
}

Telemetry RobotLogic::get_telemetry() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_telemetry;
}

void RobotLogic::read_sensors() {
    // В реальной реализации здесь будет опрос MPU6050 по I2C и ADC батареи
    // Для демонстрации генерируем плавные случайные значения
    
    static std::mt19937 gen(12345);
    static float base_roll = 0, base_pitch = 0, base_turret = 0;
    
    // Имитация небольших колебаний датчиков
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    
    m_telemetry.roll += dist(gen) * 0.1f;
    m_telemetry.pitch += dist(gen) * 0.1f;
    
    // Ограничение углов
    if (m_telemetry.roll > 90) m_telemetry.roll = 90;
    if (m_telemetry.roll < -90) m_telemetry.roll = -90;
    if (m_telemetry.pitch > 90) m_telemetry.pitch = 90;
    if (m_telemetry.pitch < -90) m_telemetry.pitch = -90;
    
    // Имитация заряда батареи (медленный разряд)
    static float battery = 12.6f;
    if (battery > 11.0f) battery -= 0.0001f;
    m_telemetry.battery_voltage = battery;
    
    // Угол башни меняется от команд
    if (std::abs(m_current_cmd.right_x) > 0.1f) {
        m_telemetry.turret_angle += m_current_cmd.right_x * 2.0f;
        if (m_telemetry.turret_angle > 180) m_telemetry.turret_angle = -180;
        if (m_telemetry.turret_angle < -180) m_telemetry.turret_angle = 180;
    }
}
