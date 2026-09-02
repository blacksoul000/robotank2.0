#include "uart_exchanger.hpp"
#include "../serial_port.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>

namespace robo_chassis {

class UartExchanger::Impl {
public:
    SerialPort* serial = nullptr;  // Ссылка на SerialPort
    int fd = -1;
    size_t package_size = 9;
    
    std::vector<uint8_t> buffer;
    std::vector<uint8_t> prefix = {0x55, 0x55};
    bool wait_prefix = true;
    
    std::thread read_thread;
    std::atomic<bool> running{false};
    
    DataCallback data_callback;
    
    bool read_data();
    bool process_buffer();
};

bool UartExchanger::Impl::read_data() {
    if (!serial || !serial->is_open()) {
        return false;
    }
    
    uint8_t temp[256];
    ssize_t bytes_read = serial->read(temp, sizeof(temp));
    
    if (bytes_read <= 0) {
        return false;
    }
    
    buffer.insert(buffer.end(), temp, temp + bytes_read);
    return true;
}

bool UartExchanger::Impl::process_buffer() {
    if (wait_prefix) {
        if (buffer.size() < prefix.size()) {
            return false;
        }
        
        // Поиск префикса в буфере
        auto it = std::search(buffer.begin(), buffer.end(), prefix.begin(), prefix.end());
        if (it == buffer.end()) {
            // Префикс не найден, удаляем лишние данные
            if (buffer.size() > prefix.size()) {
                buffer.erase(buffer.begin(), buffer.end() - prefix.size());
            }
            return false;
        }
        
        // Удаляем всё до префикса
        buffer.erase(buffer.begin(), it);
        wait_prefix = false;
    }
    
    if (!wait_prefix) {
        const size_t packet_size = prefix.size() + package_size;
        if (buffer.size() < packet_size) {
            return false;
        }
        
        wait_prefix = true;
        return true;
    }
    
    return false;
}

UartExchanger::UartExchanger(SerialPort& serial, size_t package_size) 
    : d(new Impl) {
    d->serial = &serial;
    d->package_size = package_size;
    d->buffer.reserve(256);
}

UartExchanger::~UartExchanger() {
    close();
    delete d;
}

bool UartExchanger::open() {
    if (!d->serial || !d->serial->is_open()) {
        std::cerr << "[UartExchanger] SerialPort не открыт\n";
        return false;
    }
    
    d->fd = d->serial->get_fd();
    if (d->fd < 0) {
        std::cerr << "[UartExchanger] Неверный file descriptor\n";
        return false;
    }
    
    std::cout << "[UartExchanger] Открыт для обмена данными (fd=" << d->fd << ")\n";
    d->running = true;
    d->read_thread = std::thread(&UartExchanger::read_loop, this);
    return true;
}

void UartExchanger::close() {
    if (!d->running) {
        return;
    }
    
    d->running = false;
    
    if (d->read_thread.joinable()) {
        d->read_thread.join();
    }
    d->fd = -1;
    d->serial = nullptr;

    std::cout << "[UartExchanger] Закрыт\n";
}

bool UartExchanger::is_open() const {
    return d->running;
}

void UartExchanger::set_data_callback(DataCallback callback) {
    d->data_callback = std::move(callback);
}

bool UartExchanger::send_data(const uint8_t* data, size_t len) {
    if (!d->serial || !d->serial->is_open()) {
        return false;
    }
    
    // Добавляем префикс к данным
    std::vector<uint8_t> packet;
    packet.reserve(d->prefix.size() + len);
    packet.insert(packet.end(), d->prefix.begin(), d->prefix.end());
    packet.insert(packet.end(), data, data + len);
    
    ssize_t written = d->serial->write(packet.data(), packet.size());
    if (written != static_cast<ssize_t>(packet.size())) {
        std::cerr << "[UartExchanger] Ошибка записи в порт: " 
                  << written << " байт вместо " << packet.size() << "\n";
        return false;
    }
    
    return true;
}

void UartExchanger::read_loop() {
    while (d->running) {
        if (d->read_data()) {
            if (d->process_buffer()) {
                // Извлекаем пакет данных (без префикса)
                std::vector<uint8_t> package(
                    d->buffer.begin() + d->prefix.size(),
                    d->buffer.begin() + d->prefix.size() + d->package_size
                );
                
                // Удаляем обработанный пакет из буфера
                d->buffer.erase(d->buffer.begin(), d->buffer.begin() + d->prefix.size() + d->package_size);
                
                if (d->data_callback) {
                    d->data_callback(package);
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace robo_chassis
