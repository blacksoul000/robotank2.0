#ifndef I_EXCHANGER_HPP
#define I_EXCHANGER_HPP

#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>

namespace robo_chassis {

/**
 * @brief Абстрактный интерфейс для обмена данными с Arduino
 * 
 * Позволяет использовать различные транспорты (I2C, UART) 
 * через единый интерфейс.
 */
class IExchanger {
public:
    using DataCallback = std::function<void(const std::vector<uint8_t>& data)>;
    
    virtual ~IExchanger() = default;
    
    /**
     * @brief Отправить данные на устройство
     * @param data Буфер с данными
     * @param len Длина данных
     * @return true если данные успешно отправлены
     */
    virtual bool send_data(const uint8_t* data, size_t len) = 0;
    
    /**
     * @brief Открыть соединение с устройством
     * @return true если соединение успешно установлено
     */
    virtual bool open() = 0;
    
    /**
     * @brief Закрыть соединение с устройством
     */
    virtual void close() = 0;
    
    /**
     * @brief Проверить состояние соединения
     * @return true если соединение открыто
     */
    virtual bool is_open() const = 0;
    
    /**
     * @brief Установить callback для получения данных
     * @param callback Функция обратного вызова
     */
    virtual void set_data_callback(DataCallback callback) = 0;
    
protected:
    IExchanger() = default;
};

} // namespace robo_chassis

#endif // I_EXCHANGER_HPP
