#include "i2c_master.hpp"

// Linux I2C headers
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

namespace robo_chassis {

class I2CMaster::Impl {
public:
    int fd = -1;
    std::string device;
    int slave_address = 0x04;
    size_t package_size = 9;
    int read_interval_ms = 1000;
    
    std::thread read_thread;
    std::atomic<bool> running{false};
    
    DataCallback data_callback;
    
    bool read_data(std::vector<uint8_t>& buffer);
};

bool I2CMaster::Impl::read_data(std::vector<uint8_t>& buffer) {
    if (fd < 0 || buffer.size() < package_size) {
        return false;
    }
    
    // Retry логика для обработки временных сбоев I2C
    constexpr int MAX_RETRIES = 3;
    constexpr int RETRY_DELAY_MS = 10;
    
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        ssize_t bytes_read = ::read(fd, buffer.data(), package_size);
        if (bytes_read == static_cast<ssize_t>(package_size)) {
            return true;
        }
        
        // Логирование только для последней попытки
        if (attempt == MAX_RETRIES - 1) {
            std::cerr << "[I2CMaster] Failed to read from bus after " 
                      << MAX_RETRIES << " attempts: " << std::strerror(errno) << "\n";
        } else {
            // Краткая задержка перед повторной попыткой
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
        }
    }
    
    return false;
}

I2CMaster::I2CMaster(
    const std::string& device,
    int slave_address,
    size_t package_size,
    int read_interval_ms
) : d(new Impl) {
    d->device = device;
    d->slave_address = slave_address;
    d->package_size = package_size;
    d->read_interval_ms = read_interval_ms;
}

I2CMaster::~I2CMaster() {
    close();
    delete d;
}

bool I2CMaster::open() {
    if (d->fd >= 0) {
        return true; // Already open
    }
    
    d->fd = ::open(d->device.c_str(), O_RDWR | O_NONBLOCK);
    if (d->fd < 0) {
        std::cerr << "[I2CMaster] Failed to open device " << d->device 
                  << ": " << std::strerror(errno) << "\n";
        return false;
    }
    
    if (ioctl(d->fd, I2C_SLAVE, d->slave_address) < 0) {
        std::cerr << "[I2CMaster] Failed to acquire bus access: " 
                  << std::strerror(errno) << "\n";
        ::close(d->fd);
        d->fd = -1;
        return false;
    }
    
    d->running = true;
    d->read_thread = std::thread(&I2CMaster::read_loop, this);
    
    std::cout << "[I2CMaster] Opened " << d->device 
              << " at address 0x" << std::hex << d->slave_address << std::dec << "\n";
    
    return true;
}

void I2CMaster::close() {
    if (!d->running) {
        return;
    }
    
    d->running = false;
    
    if (d->read_thread.joinable()) {
        d->read_thread.join();
    }
    
    if (d->fd >= 0) {
        ::close(d->fd);
        d->fd = -1;
    }
    
    std::cout << "[I2CMaster] Closed\n";
}

bool I2CMaster::is_open() const {
    return d->fd >= 0 && d->running;
}

void I2CMaster::set_data_callback(DataCallback callback) {
    d->data_callback = std::move(callback);
}

bool I2CMaster::send_data(const uint8_t* data, size_t len) {
    if (d->fd < 0) {
        return false;
    }
    
    ssize_t bytes_written = ::write(d->fd, data, len);
    if (bytes_written != static_cast<ssize_t>(len)) {
        std::cerr << "[I2CMaster] Failed to write to bus: " 
                  << std::strerror(errno) << "\n";
        return false;
    }
    
    return true;
}

void I2CMaster::read_loop() {
    std::vector<uint8_t> buffer(d->package_size);
    
    while (d->running) {
        if (d->read_data(buffer)) {
            if (d->data_callback) {
                d->data_callback(buffer);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(d->read_interval_ms));
    }
}

} // namespace robo_chassis
