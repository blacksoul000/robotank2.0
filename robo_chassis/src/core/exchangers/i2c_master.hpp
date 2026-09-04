#ifndef I2C_MASTER_HPP
#define I2C_MASTER_HPP

#include "i_exchanger.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

namespace robo_chassis {

/**
 * @brief Реализация обмена данными через I2C (Linux)
 * 
 * Использует linux/i2c-dev для общения с Arduino по шине I2C.
 * Периодически опрашивает устройство и вызывает callback при получении данных.
 */
class I2CMaster : public IExchanger {
public:
    /**
     * @brief Конструктор
     * @param device Путь к I2C устройству (например, "/dev/i2c-1")
     * @param slave_address Адрес ведомого устройства (по умолчанию 0x04)
     * @param package_size Размер ожидаемого пакета данных
     * @param read_interval_ms Интервал опроса устройства в миллисекундах
     */
    explicit I2CMaster(
        const std::string& device,
        int slave_address = 0x04,
        size_t package_size = 9,
        int read_interval_ms = 1000
    );
    
    ~I2CMaster() override;
    
    bool send_data(const uint8_t* data, size_t len) override;
    bool open() override;
    void close() override;
    bool is_open() const override;
    void set_data_callback(DataCallback callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> d;
    
    void read_loop();
};

} // namespace robo_chassis

#endif // I2C_MASTER_HPP
