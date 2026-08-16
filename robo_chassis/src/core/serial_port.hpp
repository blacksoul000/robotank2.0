#pragma once

#include <string>
#include <vector>
#include <cstdint>

class SerialPort {
public:
    explicit SerialPort(const std::string& device, int baudrate);
    ~SerialPort();

    bool is_open() const { return m_fd != -1; }
    
    // Запись данных в порт
    ssize_t write(const uint8_t* data, size_t length);
    
    // Чтение данных из порта (неблокирующее)
    ssize_t read(uint8_t* buffer, size_t max_length);

private:
    int m_fd = -1;
};
