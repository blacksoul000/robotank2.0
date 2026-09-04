#include "sensors/sensor_fusion.hpp"
#include "gpio/mpu6050_imu.hpp"
#include "sensors/compass_ultrasonic.hpp"
#include "logger/logger.hpp"
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>

namespace robo_chassis {

SensorFusion::SensorFusion() 
    : imu_instance_(nullptr),
      compass_instance_(nullptr),
      imu_(nullptr),
      compass_(nullptr) {}

SensorFusion::~SensorFusion() {
    // Умные указатели автоматически освободят ресурсы
}

bool SensorFusion::init() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Initializing SensorFusion...");
    
    try {
        // Создание IMU если еще не создан
        if (!imu_instance_) {
            imu_instance_ = std::make_unique<Mpu6050Imu>("/dev/i2c-1", 0x68, 0.0f);
            if (imu_instance_->init()) {
                imu_ = imu_instance_.get();
                LOG_INFO("MPU6050 initialized successfully");
            } else {
                LOG_ERROR("MPU6050 IMU not available!");
                status_ = FusionStatus::ERROR;
                return false;
            }
        }
        
        // Проверка IMU
        if (!imu_ || !imu_->isReady()) {
            LOG_ERROR("MPU6050 IMU not ready!");
            status_ = FusionStatus::ERROR;
            return false;
        }
        
        // Создание компаса если еще не создан
        if (!compass_instance_) {
            compass_instance_ = std::make_unique<sensors::Compass>(1, 0x1E);
            if (compass_instance_->init()) {
                compass_ = compass_instance_.get();
                LOG_INFO("Compass initialized successfully");
            } else {
                LOG_WARNING("Compass not available, using IMU-only mode");
            }
        }
        
        // Сохраняем указатель для локального использования
        compass_ = compass_instance_.get();
        
        // Проверка магнитометра
        magnetometer_present_ = (compass_ && compass_->isReady());
        
        if (magnetometer_present_) {
            status_ = FusionStatus::IMU_MAGNETOMETER;
            LOG_INFO("SensorFusion: IMU + Magnetometer mode");
        } else {
            status_ = FusionStatus::IMU_ONLY;
            LOG_INFO("SensorFusion: IMU-only mode (no magnetometer)");
        }
        
        initialized_ = true;
        last_update_ms_ = getCurrentTimeMs();
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during SensorFusion init: " + std::string(e.what()));
        status_ = FusionStatus::ERROR;
        return false;
    }
}

void SensorFusion::update() {
    if (!initialized_) return;
    
    auto now = getCurrentTimeMs();
    float dt = static_cast<float>(now - last_update_ms_) / 1000.0f;
    
    if (dt > 0.1f) { // Максимум 10 Гц
        LOG_WARNING("SensorFusion update skipped (dt={:.2f}s)", dt);
        return;
    }
    
    // Чтение данных с датчиков
    readIMU();
    if (magnetometer_present_) {
        readMagnetometer();
    }
    
    // Применение фильтра
    complementaryFilter();
    
    // Компенсация наклона для магнитометра
    if (magnetometer_present_) {
        compensateTilt();
    }
    
    fusion_data_.valid = true;
    last_update_ms_ = now;
}

void SensorFusion::readIMU() {
    if (!imu_) return;
    
    // Получение углов из MPU6050 через интерфейс IImu
    fusion_data_.pitch = imu_->pitch();
    fusion_data_.roll = imu_->roll();
    
    // Для комплементарного фильтра нужны сырые данные
    // Используем заглушки, т.к. прямой доступ к сырым данным не предусмотрен интерфейсом
    fusion_data_.accel_x = 0.0f;
    fusion_data_.accel_y = 0.0f;
    fusion_data_.accel_z = 0.0f;
    fusion_data_.gyro_x = 0.0f;
    fusion_data_.gyro_y = 0.0f;
    fusion_data_.gyro_z = 0.0f;
}

void SensorFusion::readMagnetometer() {
    if (!compass_) return;
    
    // Получение курса напрямую
    float heading = compass_->getHeading();
    
    // Сырые данные (если понадобятся для калибровки)
    int16_t x, y, z;
    compass_->readRaw(x, y, z);
    
    fusion_data_.mag_x = static_cast<float>(x);
    fusion_data_.mag_y = static_cast<float>(y);
    fusion_data_.mag_z = static_cast<float>(z);
}

void SensorFusion::complementaryFilter() {
    // Простой комплементарный фильтр для объединения гироскопа и акселерометра
    
    // Начальная инициализация
    static bool first_run = true;
    static float prev_pitch = 0.0f;
    static float prev_roll = 0.0f;
    static float prev_heading = 0.0f;
    
    if (first_run) {
        // Инициализация из акселерометра
        float accel_pitch = atan2(-fusion_data_.accel_x, 
                                   sqrt(fusion_data_.accel_y*fusion_data_.accel_y + 
                                        fusion_data_.accel_z*fusion_data_.accel_z)) * 180.0f / M_PI;
        float accel_roll = atan2(fusion_data_.accel_y, fusion_data_.accel_z) * 180.0f / M_PI;
        
        fusion_data_.pitch = accel_pitch;
        fusion_data_.roll = accel_roll;
        
        if (magnetometer_present_) {
            // Начальный курс из магнитометра
            float heading = atan2(-fusion_data_.mag_y, fusion_data_.mag_x) * 180.0f / M_PI;
            if (heading < 0) heading += 360.0f;
            fusion_data_.heading = heading;
            prev_heading = heading;
        } else {
            fusion_data_.heading = 0.0f;
            prev_heading = 0.0f;
        }
        
        prev_pitch = accel_pitch;
        prev_roll = accel_roll;
        first_run = false;
        return;
    }
    
    // Расчет времени между обновлениями
    float dt = 0.016f; // По умолчанию 60 Гц
    
    // Интегрирование гироскопа
    float gyro_pitch = fusion_data_.gyro_y * dt;
    float gyro_roll = -fusion_data_.gyro_z * dt;
    float gyro_yaw = fusion_data_.gyro_x * dt; // Для оси Z
    
    // Расчет углов из акселерометра
    float accel_pitch = atan2(-fusion_data_.accel_x, 
                               sqrt(fusion_data_.accel_y*fusion_data_.accel_y + 
                                    fusion_data_.accel_z*fusion_data_.accel_z)) * 180.0f / M_PI;
    float accel_roll = atan2(fusion_data_.accel_y, fusion_data_.accel_z) * 180.0f / M_PI;
    
    // Комплементарный фильтр: alpha * (prev + gyro) + (1-alpha) * accel
    fusion_data_.pitch = alpha_ * (prev_pitch + gyro_pitch) + (1.0f - alpha_) * accel_pitch;
    fusion_data_.roll = alpha_ * (prev_roll + gyro_roll) + (1.0f - alpha_) * accel_roll;
    
    // Для курса используем гироскоп + магнитометр (если есть)
    if (magnetometer_present_) {
        float mag_heading = atan2(-fusion_data_.mag_y, fusion_data_.mag_x) * 180.0f / M_PI;
        if (mag_heading < 0) mag_heading += 360.0f;
        
        // Нормализация разницы углов
        float error = mag_heading - prev_heading;
        if (error > 180.0f) error -= 360.0f;
        if (error < -180.0f) error += 360.0f;
        
        // Коррекция дрейфа гироскопа магнитометром
        fusion_data_.heading = prev_heading + gyro_yaw + 0.02f * error;
        
        // Нормализация в диапазон 0-360
        if (fusion_data_.heading < 0) fusion_data_.heading += 360.0f;
        if (fusion_data_.heading > 360.0f) fusion_data_.heading -= 360.0f;
        
        prev_heading = fusion_data_.heading;
    } else {
        // Только гироскоп (будет дрейфовать со временем)
        fusion_data_.heading = prev_heading + gyro_yaw;
        if (fusion_data_.heading < 0) fusion_data_.heading += 360.0f;
        if (fusion_data_.heading > 360.0f) fusion_data_.heading -= 360.0f;
        prev_heading = fusion_data_.heading;
    }
    
    prev_pitch = fusion_data_.pitch;
    prev_roll = fusion_data_.roll;
}

void SensorFusion::compensateTilt() {
    // Компенсация наклона для данных магнитометра
    // Преобразование координат с учетом pitch и roll
    
    float pitch_rad = fusion_data_.pitch * M_PI / 180.0f;
    float roll_rad = fusion_data_.roll * M_PI / 180.0f;
    
    float mx = fusion_data_.mag_x;
    float my = fusion_data_.mag_y;
    float mz = fusion_data_.mag_z;
    
    // Поворот вокруг оси X (roll)
    float my1 = my * cos(roll_rad) - mz * sin(roll_rad);
    float mz1 = my * sin(roll_rad) + mz * cos(roll_rad);
    
    // Поворот вокруг оси Y (pitch)
    float mx2 = mx * cos(pitch_rad) + mz1 * sin(pitch_rad);
    float mz2 = -mx * sin(pitch_rad) + mz1 * cos(pitch_rad);
    
    // Обновление компенсированных значений
    fusion_data_.mag_x = mx2;
    fusion_data_.mag_y = my1;
    fusion_data_.mag_z = mz2;
}

void SensorFusion::startCalibration() {
    calibrating_ = true;
    LOG_INFO("SensorFusion calibration started");
    
    // Сброс параметров фильтра для перекалибровки
    alpha_ = 0.98f;
}

float SensorFusion::getCalibrationProgress() const {
    if (!calibrating_) return 1.0f;
    // Упрощенная реализация - всегда 50% во время калибровки
    return 0.5f;
}

uint64_t SensorFusion::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

} // namespace robo_chassis
