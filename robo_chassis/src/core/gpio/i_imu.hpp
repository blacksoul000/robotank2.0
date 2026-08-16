#ifndef I_IMU_HPP
#define I_IMU_HPP

#include <cstdint>

namespace robo_chassis {

/**
 * @brief Абстрактный интерфейс IMU (Inertial Measurement Unit)
 */
class IImu {
public:
    virtual ~IImu() = default;

    /**
     * @brief Инициализация устройства
     * @return true если инициализация успешна
     */
    virtual bool init() = 0;

    /**
     * @brief Проверка готовности устройства
     * @return true если устройство готово к работе
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Чтение данных с датчиков
     */
    virtual void readData() = 0;

    /**
     * @brief Получить угол рыскания (yaw) в градусах
     * @return Yaw угол [0, 360)
     */
    virtual float yaw() const = 0;

    /**
     * @brief Получить угол тангажа (pitch) в градусах
     * @return Pitch угол [-90, 90]
     */
    virtual float pitch() const = 0;

    /**
     * @brief Получить угол крена (roll) в градусах
     * @return Roll угол [-180, 180]
     */
    virtual float roll() const = 0;
    
    /**
     * @brief Установить смещение yaw для калибровки
     */
    virtual void setYawOffset(float offset) = 0;

protected:
    IImu() = default;
};

} // namespace robo_chassis

#endif // I_IMU_HPP
