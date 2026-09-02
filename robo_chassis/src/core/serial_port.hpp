#pragma once

#include <string>
#include <vector>
#include <cstdint>

class SerialPort {
public:
    /**
     * @brief Конструктор с параметрами подключения и повторными попытками
     * @param device Путь к устройству (например, "/dev/ttyUSB0")
     * @param baudrate Скорость обмена в бодах
     * @param max_retries Максимальное количество попыток открытия (по умолчанию 3)
     * @param retry_delay_ms Задержка между попытками в мс (по умолчанию 500)
     */
    explicit SerialPort(const std::string& device, int baudrate, 
                       int max_retries = 3, int retry_delay_ms = 500);
    ~SerialPort();

    bool is_open() const { return m_fd != -1; }
    
    // Геттер для file descriptor (нужен для UartExchanger)
    int get_fd() const { return m_fd; }
    
    // Запись данных в порт
    ssize_t write(const uint8_t* data, size_t length);
    
    // Чтение данных из порта (неблокирующее)
    ssize_t read(uint8_t* buffer, size_t max_length);

private:
    int m_fd = -1;
};
