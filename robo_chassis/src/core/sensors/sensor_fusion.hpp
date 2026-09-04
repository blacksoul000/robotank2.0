#pragma once

#include <cstdint>
#include <string>

namespace robo_chassis {

// Результаты слияния данных сенсоров
struct FusionData {
    float heading = 0.0f;        // Курс (0-360 градусов)
    float pitch = 0.0f;          // Наклон вперед/назад (-90..+90)
    float roll = 0.0f;           // Наклон влево/вправо (-180..+180)
    
    // Сырые данные (опционально)
    float accel_x = 0.0f;
    float accel_y = 0.0f;
    float accel_z = 0.0f;
    float gyro_x = 0.0f;
    float gyro_y = 0.0f;
    float gyro_z = 0.0f;
    float mag_x = 0.0f;
    float mag_y = 0.0f;
    float mag_z = 0.0f;
    
    bool valid = false;
};

// Статус системы слияния
enum class FusionStatus {
    NOT_INITIALIZED,
    IMU_ONLY,           // Только гироскоп+акселерометр
    IMU_MAGNETOMETER,   // Полная система с магнитометром
    ERROR
};

class SensorFusion {
public:
    SensorFusion();
    ~SensorFusion() = default;
    
    // Инициализация
    bool init();
    bool isInitialized() const { return initialized_; }
    FusionStatus getStatus() const { return status_; }
    
    // Обновление данных (вызывать в цикле)
    void update();
    
    // Получение данных
    FusionData getData() const { return fusion_data_; }
    float getHeading() const { return fusion_data_.heading; }
    float getPitch() const { return fusion_data_.pitch; }
    float getRoll() const { return fusion_data_.roll; }
    
    // Проверка наличия магнитометра
    bool isMagnetometerAvailable() const { return magnetometer_present_; }
    
    // Калибровка
    void startCalibration();
    bool isCalibrating() const { return calibrating_; }
    float getCalibrationProgress() const; // 0.0 .. 1.0
    
private:
    bool initialized_ = false;
    FusionStatus status_ = FusionStatus::NOT_INITIALIZED;
    bool magnetometer_present_ = false;
    bool calibrating_ = false;
    
    FusionData fusion_data_;
    
    // Параметры комплементарного фильтра
    float alpha_ = 0.98f;  // Коэффициент доверия к гироскопу
    
    // Временные метки
    uint64_t last_update_ms_ = 0;
    
    // Внутренние методы
    void readIMU();
    void readMagnetometer();
    void complementaryFilter();
    void compensateTilt();
    
    uint64_t getCurrentTimeMs() const;
};

} // namespace robo_chassis
