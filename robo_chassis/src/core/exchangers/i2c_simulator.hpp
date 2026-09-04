#ifndef I2C_SIMULATOR_HPP
#define I2C_SIMULATOR_HPP

#include "i_exchanger.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <cmath>

namespace robo_chassis {

/**
 * @brief Симулятор I2C устройства для тестирования без реального оборудования
 * 
 * Эмулирует данные от Arduino: показания компаса, ультразвуковых датчиков,
 * состояние моторов и другую телеметрию. Используется при simulation_mode = true.
 */
class I2CSimulator : public IExchanger {
public:
    /**
     * @brief Конструктор
     * @param package_size Размер пакета данных (по умолчанию 9 байт)
     * @param read_interval_ms Интервал обновления данных в миллисекундах
     */
    explicit I2CSimulator(
        size_t package_size = 9,
        int read_interval_ms = 100
    );
    
    ~I2CSimulator() override;
    
    bool send_data(const uint8_t* data, size_t len) override;
    bool open() override;
    void close() override;
    bool is_open() const override;
    void set_data_callback(DataCallback callback) override;

private:
    class Impl;
    Impl* d;
    
    void read_loop();
    
    // Генерация симулированных данных
    std::vector<uint8_t> generate_sensor_data();
};

} // namespace robo_chassis

#endif // I2C_SIMULATOR_HPP
