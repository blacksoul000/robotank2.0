#include "sensor_manager.hpp"
#include "../logger/logger.hpp"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <numeric>

#ifdef HAVE_PIGPIO
extern "C" {
#include <pigpio.h>
}
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#endif

namespace robo {

// Глобальный флаг инициализации pigpio
#ifdef HAVE_PIGPIO
static bool g_pigpio_initialized = false;

static void ensurePigpioInitialized() {
    if (!g_pigpio_initialized) {
        int ret = gpioInitialise();
        if (ret >= 0) {
            g_pigpio_initialized = true;
        }
    }
}
#endif

SensorManager& SensorManager::getInstance() {
    static SensorManager instance;
    return instance;
}

SensorManager::~SensorManager() {
#ifdef HAVE_PIGPIO
    if (i2c_fd_ >= 0) {
        close(i2c_fd_);
    }
    if (g_pigpio_initialized) {
        gpioTerminate();
        g_pigpio_initialized = false;
    }
#endif
}

bool SensorManager::initialize(int trigger_pin, int echo_pin, uint8_t compass_addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("Инициализация менеджера датчиков...");
    
    ultrasonic_trigger_pin_ = trigger_pin;
    ultrasonic_echo_pin_ = echo_pin;
    compass_i2c_addr_ = compass_addr;
    
    // Инициализация ультразвука
    ultrasonic_available_ = initUltrasonic(trigger_pin, echo_pin);
    if (ultrasonic_available_) {
        LOG_INFO("Ультразвуковой датчик инициализирован (GPIO {}.{})", trigger_pin, echo_pin);
    } else {
        LOG_WARNING("Ультразвуковой датчик не найден");
    }
    
    // Инициализация компаса
    compass_available_ = initCompass(compass_addr);
    if (compass_available_) {
        LOG_INFO("Компас инициализирован (I2C адрес 0x{:X})", compass_addr);
    } else {
        LOG_WARNING("Компас не найден");
    }
    
    return ultrasonic_available_ || compass_available_;
}

bool SensorManager::initUltrasonic(int trigger_pin, int echo_pin) {
#ifdef HAVE_PIGPIO
    ensurePigpioInitialized();
    if (!g_pigpio_initialized) {
        LOG_ERROR("Не удалось инициализировать pigpio");
        return false;
    }
    
    gpioSetMode(trigger_pin, PI_OUTPUT);
    gpioSetMode(echo_pin, PI_INPUT);
    
    // Тестовое измерение
    auto test_distance = readUltrasonicDistance();
    return test_distance.has_value() && test_distance.value() > 0;
#else
    LOG_WARNING("pigpio доступен только на Linux/Raspberry Pi");
    return false;
#endif
}

bool SensorManager::initCompass(uint8_t i2c_addr) {
#ifdef HAVE_PIGPIO
    char filename[32];
    snprintf(filename, sizeof(filename), "/dev/i2c-%d", i2c_bus_);
    
    i2c_fd_ = open(filename, O_RDWR);
    if (i2c_fd_ < 0) {
        LOG_ERROR("Не удалось открыть I2C шину: {}", filename);
        return false;
    }
    
    if (ioctl(i2c_fd_, I2C_SLAVE, i2c_addr) < 0) {
        LOG_ERROR("Не удалось установить I2C адрес 0x{:X}", i2c_addr);
        close(i2c_fd_);
        i2c_fd_ = -1;
        return false;
    }
    
    // Попытка чтения идентификатора или тестовое чтение
    CompassData test_data;
    if (!readCompassData(test_data)) {
        close(i2c_fd_);
        i2c_fd_ = -1;
        return false;
    }
    
    return true;
#else
    LOG_WARNING("I2C доступен только на Linux/Raspberry Pi");
    return false;
#endif
}

void SensorManager::update() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    // Обновление ультразвука
    if (ultrasonic_available_) {
        auto distance = readUltrasonicDistance();
        if (distance.has_value()) {
            last_ultrasonic_.distance_cm = distance.value();
            last_ultrasonic_.timestamp = now;
            last_ultrasonic_.valid = true;
            
            // Оценка качества измерения
            if (distance.value() < 2.0f || distance.value() > 400.0f) {
                last_ultrasonic_.quality = 30;
            } else if (distance.value() < 10.0f || distance.value() > 300.0f) {
                last_ultrasonic_.quality = 70;
            } else {
                last_ultrasonic_.quality = 100;
            }
        } else {
            last_ultrasonic_.valid = false;
            last_ultrasonic_.quality = 0;
        }
    }
    
    // Обновление компаса
    if (compass_available_) {
        CompassData new_data;
        if (readCompassData(new_data)) {
            applyCompassCalibration(new_data);
            last_compass_ = new_data;
            last_compass_.timestamp = now;
            last_compass_.valid = true;
            
            // Сбор данных для калибровки
            if (compass_calibrating_ && calibration_samples_.size() < 100) {
                calibration_samples_.push_back(new_data);
                compass_calibration_progress_ = static_cast<int>(calibration_samples_.size());
                
                // Обновление мин/макс значений
                compass_min_x_ = std::min(compass_min_x_, new_data.magnetic_field_x);
                compass_max_x_ = std::max(compass_max_x_, new_data.magnetic_field_x);
                compass_min_y_ = std::min(compass_min_y_, new_data.magnetic_field_y);
                compass_max_y_ = std::max(compass_max_y_, new_data.magnetic_field_y);
            }
        } else {
            last_compass_.valid = false;
        }
    }
}

std::optional<float> SensorManager::readUltrasonicDistance() {
#ifdef HAVE_PIGPIO
    if (!g_pigpio_initialized) {
        return std::nullopt;
    }
    
    // Генерация импульса 10 мкс
    gpioWrite(ultrasonic_trigger_pin_, PI_LOW);
    gpioDelay(2);
    gpioWrite(ultrasonic_trigger_pin_, PI_HIGH);
    gpioDelay(10);
    gpioWrite(ultrasonic_trigger_pin_, PI_LOW);
    
    // Ожидание эха с таймаутом
    uint32_t pulse_start = 0;
    uint32_t pulse_end = 0;
    int timeout = 30000; // 30 мс таймаут
    
    // Ждем нарастающего фронта
    for (int i = 0; i < timeout; i++) {
        if (gpioRead(ultrasonic_echo_pin_) == PI_HIGH) {
            pulse_start = gpioTick();
            break;
        }
        gpioDelay(1);
    }
    
    if (pulse_start == 0) {
        return std::nullopt;
    }
    
    // Ждем спадающего фронта
    for (int i = 0; i < timeout; i++) {
        if (gpioRead(ultrasonic_echo_pin_) == PI_LOW) {
            pulse_end = gpioTick();
            break;
        }
        gpioDelay(1);
    }
    
    if (pulse_end == 0) {
        return std::nullopt;
    }
    
    // Расчет расстояния
    uint32_t duration = pulse_end - pulse_start;
    float distance = (duration / 29.1f) / 2.0f; // см
    
    // Фильтрация неверных значений
    if (distance < 2.0f || distance > 400.0f) {
        return std::nullopt;
    }
    
    return distance;
#else
    return std::nullopt;
#endif
}

bool SensorManager::readCompassData(CompassData& data) {
#ifdef HAVE_PIGPIO
    if (i2c_fd_ < 0) {
        return false;
    }
    
    // Чтение данных зависит от конкретной модели компаса
    // Здесь приведен пример для HMC5883L
    
    // Установка регистра адреса для чтения
    uint8_t reg = 0x03; // Начальный регистр данных
    if (write(i2c_fd_, &reg, 1) != 1) {
        return false;
    }
    
    // Чтение 6 байт данных (X, Z, Y по 2 байта каждый)
    uint8_t buffer[6];
    if (read(i2c_fd_, buffer, 6) != 6) {
        return false;
    }
    
    // Преобразование в значения (HMC5883L)
    int16_t raw_x = (static_cast<int16_t>(buffer[0]) << 8) | buffer[1];
    int16_t raw_z = (static_cast<int16_t>(buffer[2]) << 8) | buffer[3];
    int16_t raw_y = (static_cast<int16_t>(buffer[4]) << 8) | buffer[5];
    
    // Масштабирование (зависит от настроек усиления, по умолчанию ±1.3 Гаусс)
    const float scale = 0.92f; // мкТл на единицу
    data.magnetic_field_x = raw_x * scale;
    data.magnetic_field_y = raw_y * scale;
    data.magnetic_field_z = raw_z * scale;
    
    // Расчет курса
    data.heading_deg = std::atan2(data.magnetic_field_y, data.magnetic_field_x) * 180.0f / M_PI;
    if (data.heading_deg < 0) {
        data.heading_deg += 360.0f;
    }
    
    data.tilt_angle = 0.0f; // Можно рассчитать через акселерометр если есть
    
    return true;
#else
    return false;
#endif
}

void SensorManager::applyCompassCalibration(CompassData& data) {
    if (!compass_available_ || calibration_samples_.empty()) {
        return;
    }
    
    // Применение простой калибровки (hard iron correction)
    float offset_x = (compass_min_x_ + compass_max_x_) / 2.0f;
    float offset_y = (compass_min_y_ + compass_max_y_) / 2.0f;
    
    data.magnetic_field_x -= offset_x;
    data.magnetic_field_y -= offset_y;
    
    // Пересчет курса с учетом калибровки
    data.heading_deg = std::atan2(data.magnetic_field_y, data.magnetic_field_x) * 180.0f / M_PI;
    if (data.heading_deg < 0) {
        data.heading_deg += 360.0f;
    }
}

UltrasonicData SensorManager::getUltrasonicData() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_ultrasonic_;
}

CompassData SensorManager::getCompassData() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_compass_;
}

void SensorManager::startCompassCalibration() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!compass_available_) {
        return;
    }
    
    calibration_samples_.clear();
    compass_min_x_ = 1000.0f;
    compass_max_x_ = -1000.0f;
    compass_min_y_ = 1000.0f;
    compass_max_y_ = -1000.0f;
    compass_calibrating_ = true;
    compass_calibration_progress_ = 0;
    
    LOG_INFO("Запущена калибровка компаса. Вращайте робота...");
}

bool SensorManager::finishCompassCalibration() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!compass_calibrating_) {
        return false;
    }
    
    if (calibration_samples_.size() < 20) {
        LOG_WARNING("Недостаточно данных для калибровки компаса: {}", calibration_samples_.size());
        compass_calibrating_ = false;
        return false;
    }
    
    compass_calibrating_ = false;
    LOG_INFO("Калибровка компаса завершена. Собрано образцов: {}", calibration_samples_.size());
    LOG_INFO("Диапазон X: [{:.2f}, {:.2f}], Y: [{:.2f}, {:.2f}]", 
             compass_min_x_, compass_max_x_, compass_min_y_, compass_max_y_);
    
    return true;
}

} // namespace robo
