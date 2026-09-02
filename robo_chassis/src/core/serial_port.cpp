#include "serial_port.hpp"
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>

SerialPort::SerialPort(const std::string& device, int baudrate, int max_retries, int retry_delay_ms) {
    int retries = 0;
    
    while (retries < max_retries) {
        m_fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        
        if (m_fd != -1) {
            break;
        }
        
        retries++;
        if (retries < max_retries) {
            std::cerr << "Попытка " << retries << "/" << max_retries 
                      << ": Не удалось открыть порт " << device 
                      << " (" << strerror(errno) << "). Повтор через " 
                      << retry_delay_ms << " мс...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
        }
    }
    
    if (m_fd == -1) {
        std::cerr << "Предупреждение: Не удалось открыть порт " << device 
                  << " после " << max_retries << " попыток (" << strerror(errno) 
                  << "). Работа без Arduino.\n";
        return;
    }

    struct termios tty;
    if (tcgetattr(m_fd, &tty) != 0) {
        std::cerr << "Ошибка получения атрибутов порта\n";
        close(m_fd);
        m_fd = -1;
        return;
    }

    // Настройка скорости (поддержка стандартных скоростей)
    speed_t speed = B115200;
    if (baudrate == 9600) speed = B9600;
    else if (baudrate == 19200) speed = B19200;
    else if (baudrate == 38400) speed = B38400;
    else if (baudrate == 57600) speed = B57600;
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    // 8N1
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | CSTOPB);

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        std::cerr << "Ошибка настройки порта\n";
        close(m_fd);
        m_fd = -1;
        return;
    }

    std::cout << "Порт " << device << " открыт успешно (" << baudrate << " бод).\n";
}

SerialPort::~SerialPort() {
    if (m_fd != -1) {
        close(m_fd);
    }
}

ssize_t SerialPort::write(const uint8_t* data, size_t length) {
    if (m_fd == -1) return -1;
    return ::write(m_fd, data, length);
}

ssize_t SerialPort::read(uint8_t* buffer, size_t max_length) {
    if (m_fd == -1) return -1;
    return ::read(m_fd, buffer, max_length);
}
