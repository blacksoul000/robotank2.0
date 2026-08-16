#include "sensors/compass_ultrasonic.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>
#include <thread>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <atomic>

// Для работы с GPIO на Raspberry Pi через pigpio (без демона)
#ifdef HAVE_PIGPIO
extern "C" {
#include <pigpio.h>
}
#endif

namespace robo_chassis {
namespace sensors {

// Глобальный флаг инициализации pigpio с мьютексом для потокобезопасности
#ifdef HAVE_PIGPIO
static std::atomic<bool> g_pigpio_initialized{false};
static std::mutex g_pigpio_init_mutex;
static std::atomic<int> g_pigpio_init_result{-1};

static bool ensurePigpioInitialized() {
    // Быстрая проверка без блокировки, если уже инициализировано
    if (g_pigpio_initialized.load(std::memory_order_acquire)) {
        return g_pigpio_init_result.load() >= 0;
    }
    
    // Блокировка для безопасной инициализации
    std::lock_guard<std::mutex> lock(g_pigpio_init_mutex);
    
    // Двойная проверка после захвата мьютекса
    if (g_pigpio_initialized.load(std::memory_order_acquire)) {
        return g_pigpio_init_result.load() >= 0;
    }
    
    // Инициализация pigpio
    int ret = gpioInitialise();
    g_pigpio_init_result.store(ret, std::memory_order_release);
    
    if (ret >= 0) {
        g_pigpio_initialized.store(true, std::memory_order_release);
        return true;
    } else {
        // Логирование ошибки инициализации GPIO
        // В реальном проекте здесь должен быть вызов логгера
        return false;
    }
}

static void cleanupPigpio() {
    if (g_pigpio_initialized.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(g_pigpio_init_mutex);
        if (g_pigpio_initialized.load(std::memory_order_acquire)) {
            gpioTerminate();
            g_pigpio_initialized.store(false, std::memory_order_release);
            g_pigpio_init_result.store(-1, std::memory_order_release);
        }
    }
}
#endif

// ============================================================================
// Compass Implementation
// ============================================================================

Compass::Compass(int i2c_bus, int addr) 
    : fd_(-1), address_(addr), ready_(false), calibrated_(false),
      last_raw_x_(0), last_raw_y_(0), last_raw_z_(0) {
    cal_min_[0] = cal_min_[1] = cal_min_[2] = -400.0f;
    cal_max_[0] = cal_max_[1] = cal_max_[2] = 400.0f;
}

Compass::~Compass() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool Compass::init() {
    char filename[32];
    snprintf(filename, sizeof(filename), "/dev/i2c-%d", 1); // RPi I2C bus 1
    
    if ((fd_ = open(filename, O_RDWR)) < 0) {
        return false;
    }

    if (ioctl(fd_, I2C_SLAVE, address_) < 0) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    // Инициализация для HMC5883L
    uint8_t config_a = 0x70; // 8 averages, 15 Hz default
    uint8_t config_b = 0xA0; // Gain 1.3 Ga
    uint8_t mode = 0x00;     // Continuous measurement

    if (write(fd_, &config_a, 1) != 1 ||
        write(fd_, &config_b, 1) != 1 ||
        write(fd_, &mode, 1) != 1) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ready_ = true;
    return true;
}

bool Compass::readRaw(int16_t& x, int16_t& y, int16_t& z) {
    if (!ready_) return false;
    
    uint8_t buffer[7];
    buffer[0] = 0x03; // Start register for X
    
    if (write(fd_, buffer, 1) != 1) return false;
    
    if (read(fd_, buffer + 1, 6) != 6) return false;

    // HMC5883L: X, Z, Y порядок
    last_raw_x_ = (buffer[1] << 8) | buffer[2];
    last_raw_z_ = (buffer[3] << 8) | buffer[4];
    last_raw_y_ = (buffer[5] << 8) | buffer[6];
    
    x = last_raw_x_;
    y = last_raw_y_;
    z = last_raw_z_;
    
    return true;
}

float Compass::getHeading() {
    if (!ready_) return 0.0f;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    int16_t raw_x, raw_y, raw_z;
    if (!readRaw(raw_x, raw_y, raw_z)) {
        return 0.0f;
    }

    // Применение калибровки (Hard Iron Compensation)
    float x = static_cast<float>(raw_x);
    float y = static_cast<float>(raw_y);
    float z = static_cast<float>(raw_z);

    if (calibrated_) {
        // Нормализация к диапазону [-400, 400]
        x = (x - cal_min_[0]) / (cal_max_[0] - cal_min_[0]) * 800.0f - 400.0f;
        y = (y - cal_min_[1]) / (cal_max_[1] - cal_min_[1]) * 800.0f - 400.0f;
        z = (z - cal_min_[2]) / (cal_max_[2] - cal_min_[2]) * 800.0f - 400.0f;
    }

    // Расчет курса
    float heading = calculateHeading(x, y, z);
    
    // Фильтрация
    applyFilter(heading);
    
    return heading;
}

void Compass::applyFilter(float& heading) {
    // Обработка перехода через 360/0 для корректного усреднения
    if (!heading_history_.empty()) {
        float last = heading_history_.back();
        if (heading < 10.0f && last > 350.0f) {
            heading += 360.0f;
        } else if (heading > 350.0f && last < 10.0f) {
            heading -= 360.0f;
        }
    }
    
    heading_history_.push_back(heading);
    
    if (static_cast<int>(heading_history_.size()) > FILTER_WINDOW) {
        heading_history_.pop_front();
    }
    
    // Скользящее среднее
    float sum = 0.0f;
    for (float h : heading_history_) {
        sum += h;
    }
    
    heading = sum / static_cast<float>(heading_history_.size());
    
    // Нормализация обратно к 0-360
    if (heading < 0.0f) heading += 360.0f;
    if (heading >= 360.0f) heading -= 360.0f;
}

float Compass::calculateHeading(float x, float y, float z) {
    // Учет наклона (упрощенно, без акселерометра)
    // Для полной компенсации наклона нужны данные с гироскопа/акселерометра
    float heading = atan2f(y, x);
    
    // Магнитное склонение для Москвы ~10° восточное
    const float declination = 10.0f * M_PI / 180.0f;
    heading += declination;

    // Нормализация 0-360
    if (heading < 0) heading += 2 * M_PI;
    if (heading > 2 * M_PI) heading -= 2 * M_PI;

    return heading * 180.0f / M_PI;
}

void Compass::setCalibration(float min_x, float max_x, float min_y, float max_y, 
                             float min_z, float max_z) {
    std::lock_guard<std::mutex> lock(mutex_);
    cal_min_[0] = min_x; cal_max_[0] = max_x;
    cal_min_[1] = min_y; cal_max_[1] = max_y;
    cal_min_[2] = min_z; cal_max_[2] = max_z;
    calibrated_ = true;
}

std::string Compass::getCalibrationStatus() const {
    return calibrated_ ? "CALIBRATED" : "NOT_CALIBRATED";
}

// ============================================================================
// Ultrasonic Implementation
// ============================================================================

Ultrasonic::Ultrasonic(int trig_pin, int echo_pin)
    : trigger_pin_(trig_pin), echo_pin_(echo_pin), ready_(false), max_distance_(400.0f) {
}

Ultrasonic::~Ultrasonic() {
    // Очистка GPIO не требуется при выходе
}

bool Ultrasonic::init() {
#ifdef HAVE_PIGPIO
    if (!ensurePigpioInitialized()) {
        // Ошибка инициализации GPIO - ультразвуковой датчик не будет работать
        ready_ = false;
        return false;
    }

    // Настройка пинов: Trigger - OUTPUT, Echo - INPUT
    gpioSetMode(trigger_pin_, PI_OUTPUT);
    gpioSetMode(echo_pin_, PI_INPUT);
    
    // Начальное состояние Trigger - LOW
    gpioWrite(trigger_pin_, 0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ready_ = true;
    return true;
#else
    // pigpio недоступен, ультразвуковой датчик не будет работать
    ready_ = false;
    return false;
#endif
}

bool Ultrasonic::trigger() {
#ifdef HAVE_PIGPIO
    if (!g_pigpio_initialized.load(std::memory_order_acquire)) return false;
    
    // Генерация импульса 10 мкс
    gpioWrite(trigger_pin_, 1);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    gpioWrite(trigger_pin_, 0);
    return true;
#else
    return false;
#endif
}

float Ultrasonic::measureEchoDuration() {
#ifdef HAVE_PIGPIO
    if (!g_pigpio_initialized.load(std::memory_order_acquire)) return -1.0f;
    
    // Таймауты для защиты от зависания (HC-SR04 макс. дистанция ~4-5м = ~30мс)
    // Добавляем запас и используем 50мс как максимальный таймаут
    constexpr uint32_t ECHO_TIMEOUT_US = 50000; // 50мс в микросекундах
    constexpr uint32_t START_TIMEOUT_US = 5000; // 5мс на ожидание старта
    
    uint32_t start = gpioTick();
    
    // Ждем пока echo не станет HIGH (с таймаутом)
    while (gpioRead(echo_pin_) == 0) {
        if ((gpioTick() - start) > START_TIMEOUT_US) {
            return -1.0f; // Таймаут ожидания старта
        }
    }
    
    uint32_t pulse_start = gpioTick();
    
    // Ждем пока echo не станет LOW (с таймаутом)
    while (gpioRead(echo_pin_) == 1) {
        if ((gpioTick() - pulse_start) > ECHO_TIMEOUT_US) {
            return -1.0f; // Таймаут ожидания окончания импульса
        }
    }
    
    uint32_t pulse_end = gpioTick();
    return static_cast<float>(pulse_end - pulse_start);
#else
    return -1.0f;
#endif
}

float Ultrasonic::readDistanceCm() {
    if (!ready_) return -1.0f;
    
    std::lock_guard<std::mutex> lock(mutex_);

    if (!trigger()) return -1.0f;
    
    float duration_us = measureEchoDuration();
    if (duration_us < 0) return -1.0f;
    
    // Скорость звука ~340 m/s = 0.034 cm/us
    // Расстояние = (duration * 0.034) / 2 (туда и обратно)
    float distance_cm = (duration_us * 0.034f) / 2.0f;
    
    if (distance_cm > max_distance_ || distance_cm < 2.0f) {
        return -1.0f; // Вне диапазона
    }
    
    return distance_cm;
}

void Ultrasonic::setMaxDistanceCm(float max_dist) {
    max_distance_ = max_dist;
}

} // namespace sensors
} // namespace robo_chassis
