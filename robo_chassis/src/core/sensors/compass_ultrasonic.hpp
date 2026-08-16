#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cmath>
#include <array>
#include <deque>

namespace robo_chassis {
namespace sensors {

/**
 * @brief Класс для работы с магнитометром (Компас)
 * Поддерживаемые чипы: HMC5883L, QMC5883L
 * Интерфейс: I2C
 * 
 * Особенности:
 * - Внутренняя обработка сырых данных X/Y/Z (не передаются в UI)
 * - Компенсация Hard Iron (смещение)
 * - Фильтрация скользящим средним для уменьшения дрифта
 * - Обработка перехода через 360/0 градусов
 */
class Compass {
public:
    Compass(int i2c_bus = 1, int address = 0x1E);
    ~Compass();

    bool init();
    bool isReady() const { return ready_; }
    
    // Чтение только курса (сырые данные используются внутри)
    float getHeading();
    
    // Калибровка
    void setCalibration(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z);
    std::string getCalibrationStatus() const;
    
    // Получение сырых данных (для отладки/калибровки)
    bool readRaw(int16_t& x, int16_t& y, int16_t& z);

private:
    int fd_;
    int address_;
    bool ready_;
    float cal_min_[3], cal_max_[3];
    bool calibrated_;
    std::mutex mutex_;
    
    // Фильтрация
    static constexpr int FILTER_WINDOW = 10;
    std::deque<float> heading_history_;
    
    // Внутренние данные
    int16_t last_raw_x_, last_raw_y_, last_raw_z_;
    
    float calculateHeading(float x, float y, float z);
    void applyFilter(float& heading);
};

/**
 * @brief Класс для работы с ультразвуковым дальномером
 * Поддерживаемые: HC-SR04
 * Интерфейс: GPIO (Trigger/Echo)
 */
class Ultrasonic {
public:
    Ultrasonic(int trigger_pin, int echo_pin);
    ~Ultrasonic();

    bool init();
    bool isReady() const { return ready_; }

    // Чтение дистанции в см
    float readDistanceCm();
    
    // Настройки
    void setMaxDistanceCm(float max_dist);
    float getMaxDistanceCm() const { return max_distance_; }

private:
    int trigger_pin_;
    int echo_pin_;
    bool ready_;
    float max_distance_;
    std::mutex mutex_;

    bool trigger();
    float measureEchoDuration();
};

} // namespace sensors
} // namespace robo_chassis
