#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <mutex>

namespace robo {

/**
 * @brief Структура данных датчика расстояния (ультразвук)
 */
struct UltrasonicData {
    float distance_cm;        // Расстояние в см
    uint8_t quality;          // Качество измерения (0-100)
    std::chrono::steady_clock::time_point timestamp;
    bool valid = false;
};

/**
 * @brief Структура данных компаса (магнитометр)
 */
struct CompassData {
    float heading_deg;        // Курс в градусах (0-360)
    float magnetic_field_x;   // Магнитное поле X (мкТл)
    float magnetic_field_y;   // Магнитное поле Y (мкТл)
    float magnetic_field_z;   // Магнитное поле Z (мкТл)
    float tilt_angle;         // Угол наклона (градусы)
    std::chrono::steady_clock::time_point timestamp;
    bool valid = false;
};

/**
 * @brief Менеджер внешних датчиков (ультразвук, компас)
 * 
 * Поддерживает:
 * - HC-SR04 (ультразвуковой дальномер) через GPIO
 * - HMC5883L / QMC5883L (цифровой компас) через I2C
 * - MPU9250 (IMU с компасом) через I2C
 */
class SensorManager {
public:
    static SensorManager& getInstance();
    
    // Запрет копирования
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;

    /**
     * @brief Инициализация менеджера датчиков
     * @param ultrasonic_trigger_pin GPIO пин триггера ультразвука
     * @param ultrasonic_echo_pin GPIO пин эха ультразвука
     * @param compass_i2c_addr I2C адрес компаса (по умолчанию 0x1E для HMC5883L)
     * @return true если инициализация успешна
     */
    bool initialize(int ultrasonic_trigger_pin = 23, 
                    int ultrasonic_echo_pin = 24,
                    uint8_t compass_i2c_addr = 0x1E);

    /**
     * @brief Обновление данных со всех датчиков
     * Вызывать периодически в основном цикле
     */
    void update();

    /**
     * @brief Получить последние данные ультразвука
     */
    UltrasonicData getUltrasonicData() const;

    /**
     * @brief Получить последние данные компаса
     */
    CompassData getCompassData() const;

    /**
     * @brief Проверка доступности ультразвукового датчика
     */
    bool isUltrasonicAvailable() const { return ultrasonic_available_; }

    /**
     * @brief Проверка доступности компаса
     */
    bool isCompassAvailable() const { return compass_available_; }

    /**
     * @brief Калибровка компаса (запуск сбора данных для калибровки)
     */
    void startCompassCalibration();

    /**
     * @brief Завершение калибровки компаса
     * @return true если калибровка успешна
     */
    bool finishCompassCalibration();

    /**
     * @brief Статус калибровки компаса
     */
    bool isCompassCalibrating() const { return compass_calibrating_; }

    /**
     * @brief Получение статуса калибровки компаса (0-100%)
     */
    int getCompassCalibrationProgress() const { return compass_calibration_progress_; }

private:
    SensorManager() = default;
    ~SensorManager();

    // Инициализация ультразвука
    bool initUltrasonic(int trigger_pin, int echo_pin);
    
    // Инициализация компаса
    bool initCompass(uint8_t i2c_addr);
    
    // Чтение ультразвука
    std::optional<float> readUltrasonicDistance();
    
    // Чтение компаса
    bool readCompassData(CompassData& data);
    
    // Обработка данных компаса с учетом калибровки
    void applyCompassCalibration(CompassData& data);

    mutable std::mutex mutex_;
    
    // Конфигурация
    int ultrasonic_trigger_pin_ = -1;
    int ultrasonic_echo_pin_ = -1;
    uint8_t compass_i2c_addr_ = 0;
    int i2c_bus_ = 1; // /dev/i2c-1 на Raspberry Pi
    
    // Состояние
    bool ultrasonic_available_ = false;
    bool compass_available_ = false;
    bool compass_calibrating_ = false;
    int compass_calibration_progress_ = 0;
    
    // Последние данные
    UltrasonicData last_ultrasonic_;
    CompassData last_compass_;
    
    // Данные для калибровки компаса (резервирование памяти для избежания реаллокаций)
    std::vector<CompassData> calibration_samples_;
    float compass_min_x_ = -1000.0f;
    float compass_max_x_ = 1000.0f;
    float compass_min_y_ = -1000.0f;
    float compass_max_y_ = 1000.0f;
    
    // Файловый дескриптор I2C
    int i2c_fd_ = -1;
};

} // namespace robo
