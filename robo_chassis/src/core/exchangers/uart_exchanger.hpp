#ifndef UART_EXCHANGER_HPP
#define UART_EXCHANGER_HPP

#include "i_exchanger.hpp"
#include "../serial_port.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <cstdint>

namespace robo_chassis {

/**
 * @brief Реализация обмена данными через UART
 * 
 * Использует существующий SerialPort для общения с Arduino.
 * Поддерживает префикс 0x55 0x55 для синхронизации пакетов.
 */
class UartExchanger : public IExchanger {
public:
    /**
     * @brief Конструктор
     * @param serial Ссылка на инициализированный SerialPort
     * @param package_size Размер ожидаемого пакета данных
     */
    explicit UartExchanger(SerialPort& serial, size_t package_size = 9);
    
    ~UartExchanger() override;
    
    bool send_data(const uint8_t* data, size_t len) override;
    bool open() override;
    void close() override;
    bool is_open() const override;
    void set_data_callback(DataCallback callback) override;

private:
    class Impl;
    Impl* d;
    
    void read_loop();
};

} // namespace robo_chassis

#endif // UART_EXCHANGER_HPP
