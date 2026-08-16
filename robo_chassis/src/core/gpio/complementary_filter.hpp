#ifndef COMPLEMENTARY_FILTER_HPP
#define COMPLEMENTARY_FILTER_HPP

#include <cmath>

namespace robo_chassis {

/**
 * @brief Комплементарный фильтр для объединения данных акселерометра и гироскопа
 */
class ComplementaryFilter {
public:
    ComplementaryFilter();
    ~ComplementaryFilter();

    /**
     * @brief Включить/выключить использование магнитометра
     */
    void setUseMag(bool use);

    /**
     * @brief Установить данные гироскопа
     * @param gx Гироскоп по оси X (градусы/сек)
     * @param gy Гироскоп по оси Y (градусы/сек)
     * @param gz Гироскоп по оси Z (градусы/сек)
     */
    void setGyroData(float gx, float gy, float gz);

    /**
     * @brief Установить данные акселерометра
     * @param ax Акселерометр по оси X
     * @param ay Акселерометр по оси Y
     * @param az Акселерометр по оси Z
     */
    void setAccelData(float ax, float ay, float az);

    /**
     * @brief Установить данные магнитометра
     * @param mx Магнитометр по оси X
     * @param my Магнитометр по оси Y
     * @param mz Магнитометр по оси Z
     */
    void setMagData(float mx, float my, float mz);

    /**
     * @brief Обработать данные и обновить углы
     * @param dt Время с последнего вызова в секундах
     */
    void process(float dt);

    /**
     * @brief Сбросить все данные и углы
     */
    void reset();

    /**
     * @brief Получить угол тангажа (pitch)
     * @return Pitch в градусах
     */
    float pitch() const;

    /**
     * @brief Получить угол крена (roll)
     * @return Roll в градусах
     */
    float roll() const;

    /**
     * @brief Получить угол рыскания (yaw)
     * @return Yaw в градусах
     */
    float yaw() const;

private:
    class Impl;
    Impl* d;
};

} // namespace robo_chassis

#endif // COMPLEMENTARY_FILTER_HPP
