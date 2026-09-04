#include "mpu6050_imu.hpp"
#include "logger.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <chrono>
#include <cmath>

namespace robo_chassis {

namespace {
    // MPU6050 Register addresses
    constexpr uint8_t MPU6050_SMPLRT_DIV = 0x19;
    constexpr uint8_t MPU6050_CONFIG = 0x1A;
    constexpr uint8_t MPU6050_GYRO_CONFIG = 0x1B;
    constexpr uint8_t MPU6050_ACCEL_CONFIG = 0x1C;
    constexpr uint8_t MPU6050_PWR_MGMT_1 = 0x6B;
    constexpr uint8_t MPU6050_PWR_MGMT_2 = 0x6C;
    constexpr uint8_t MPU6050_INT_ENABLE = 0x38;
    
    constexpr uint8_t MPU6050_ACCEL_XOUT_H = 0x3B;
    constexpr uint8_t MPU6050_GYRO_XOUT_H = 0x43;
    
    constexpr uint8_t MPU6050_WHO_AM_I = 0x75;
    constexpr uint8_t MPU6050_WHO_AM_I_VALUE = 0x68;
    
    constexpr int I2C_READ_DELAY_US = 1000;
    
    // Коэффициенты масштабирования
    constexpr float ACCEL_SCALE = 16384.0f;  // +/- 2g
    constexpr float GYRO_SCALE = 131.0f;     // +/- 250 deg/s
}

class Mpu6050Imu::Impl {
public:
    std::chrono::steady_clock::time_point last_read_time;
    float dt = 0.016f;  // ~60Hz по умолчанию
};

Mpu6050Imu::Mpu6050Imu(const std::string& i2c_device, int address, float yaw_offset)
    : m_fd(-1)
    , m_address(address)
    , m_device(i2c_device)
    , m_yaw_offset(yaw_offset)
    , m_initialized(false)
    , m_ready(false)
    , d(new Impl)
{
    m_ax = m_ay = m_az = 0.0f;
    m_gx = m_gy = m_gz = 0.0f;
}

Mpu6050Imu::~Mpu6050Imu() {
    closeI2c();
    delete d;
}

bool Mpu6050Imu::openI2c() {
    if (m_fd >= 0) {
        return true;
    }
    
    m_fd = ::open(m_device.c_str(), O_RDWR);
    if (m_fd < 0) {
        LOG_ERROR_SRC("Failed to open " + m_device + ": " + std::strerror(errno), "mpu6050_imu");
        return false;
    }
    
    if (ioctl(m_fd, I2C_SLAVE, m_address) < 0) {
        LOG_ERROR_SRC("Failed to set slave address: " + std::string(std::strerror(errno)), "mpu6050_imu");
        closeI2c();
        return false;
    }
    
    return true;
}

void Mpu6050Imu::closeI2c() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool Mpu6050Imu::writeRegister(uint8_t reg, uint8_t value) {
    if (m_fd < 0) return false;
    
    uint8_t buffer[2] = {reg, value};
    ssize_t written = ::write(m_fd, buffer, 2);
    return written == 2;
}

bool Mpu6050Imu::readRegisters(uint8_t reg, uint8_t* data, size_t len) {
    if (m_fd < 0 || !data || len == 0) return false;
    
    // Запись адреса регистра
    ssize_t written = ::write(m_fd, &reg, 1);
    if (written != 1) {
        return false;
    }
    
    // Чтение данных
    ssize_t read_bytes = ::read(m_fd, data, len);
    return read_bytes == static_cast<ssize_t>(len);
}

bool Mpu6050Imu::init() {
    if (!openI2c()) {
        return false;
    }
    
    // Проверка WHO_AM_I
    uint8_t who_am_i = 0;
    if (!readRegisters(MPU6050_WHO_AM_I, &who_am_i, 1)) {
        LOG_ERROR_SRC("Failed to read WHO_AM_I register", "mpu6050_imu");
        closeI2c();
        return false;
    }
    
    if (who_am_i != MPU6050_WHO_AM_I_VALUE) {
        LOG_ERROR_SRC("Invalid WHO_AM_I value: 0x" + 
                      std::to_string(who_am_i), "mpu6050_imu");
        closeI2c();
        return false;
    }
    
    LOG_INFO_SRC("MPU6050 detected at address 0x" + std::to_string(m_address), "mpu6050_imu");
    
    // Wake up MPU6050
    if (!writeRegister(MPU6050_PWR_MGMT_1, 0x00)) {
        LOG_ERROR_SRC("Failed to wake up MPU6050", "mpu6050_imu");
        closeI2c();
        return false;
    }
    
    // Настройка фильтра низких частот (DLPF)
    if (!writeRegister(MPU6050_CONFIG, 0x03)) {  // DLPF_CFG = 3, 44Hz
        LOG_ERROR_SRC("Failed to configure DLPF", "mpu6050_imu");
        closeI2c();
        return false;
    }
    
    // Настройка гироскопа: +/- 250 deg/s
    if (!writeRegister(MPU6050_GYRO_CONFIG, 0x00)) {
        LOG_ERROR_SRC("Failed to configure gyro scale", "mpu6050_imu");
        closeI2c();
        return false;
    }
    
    // Настройка акселерометра: +/- 2g
    if (!writeRegister(MPU6050_ACCEL_CONFIG, 0x00)) {
        LOG_ERROR_SRC("Failed to configure accel scale", "mpu6050_imu");
        closeI2c();
        return false;
    }
    
    // Частота дискретизации: 1kHz / (1 + SMPLRT_DIV) = 125Hz
    if (!writeRegister(MPU6050_SMPLRT_DIV, 0x07)) {
        LOG_ERROR_SRC("Failed to configure sample rate", "mpu6050_imu");
        closeI2c();
        return false;
    }
    
    // Калибровка гироскопа
    calibrateGyro();
    
    m_initialized = true;
    m_ready = false;  // Будет готов после первого чтения
    
    d->last_read_time = std::chrono::steady_clock::now();
    
    LOG_INFO_SRC("Initialized successfully", "mpu6050_imu");
    return true;
}

void Mpu6050Imu::calibrateGyro() {
    LOG_INFO_SRC("Calibrating gyro...", "mpu6050_imu");
    
    // Считываем несколько значений для усреднения
    constexpr int samples = 100;
    float gx_sum = 0, gy_sum = 0, gz_sum = 0;
    
    for (int i = 0; i < samples; ++i) {
        readRawData();
        gx_sum += m_gx;
        gy_sum += m_gy;
        gz_sum += m_gz;
        usleep(1000);
    }
    
    // Сохраняем смещения (в данном случае просто игнорируем их в readData)
    float gx_bias = gx_sum / samples;
    float gy_bias = gy_sum / samples;
    float gz_bias = gz_sum / samples;
    
    LOG_INFO_SRC("Gyro bias: X=" + std::to_string(gx_bias) + 
                 ", Y=" + std::to_string(gy_bias) + 
                 ", Z=" + std::to_string(gz_bias) + " deg/s", "mpu6050_imu");
}

bool Mpu6050Imu::isReady() const {
    return m_ready;
}

void Mpu6050Imu::readRawData() {
    uint8_t buffer[14];
    
    if (!readRegisters(MPU6050_ACCEL_XOUT_H, buffer, 14)) {
        LOG_ERROR_SRC("Failed to read sensor data", "mpu6050_imu");
        return;
    }
    
    // Акселерометр
    int16_t ax_raw = (buffer[0] << 8) | buffer[1];
    int16_t ay_raw = (buffer[2] << 8) | buffer[3];
    int16_t az_raw = (buffer[4] << 8) | buffer[5];
    
    // Гироскоп
    int16_t gx_raw = (buffer[6] << 8) | buffer[7];
    int16_t gy_raw = (buffer[8] << 8) | buffer[9];
    int16_t gz_raw = (buffer[10] << 8) | buffer[11];
    
    // Преобразование в физические величины
    m_ax = static_cast<float>(ax_raw) / ACCEL_SCALE;
    m_ay = static_cast<float>(ay_raw) / ACCEL_SCALE;
    m_az = static_cast<float>(az_raw) / ACCEL_SCALE;
    
    m_gx = static_cast<float>(gx_raw) / GYRO_SCALE;
    m_gy = static_cast<float>(gy_raw) / GYRO_SCALE;
    m_gz = static_cast<float>(gz_raw) / GYRO_SCALE;
}

void Mpu6050Imu::readData() {
    if (!m_initialized) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        now - d->last_read_time).count();
    
    d->dt = static_cast<float>(elapsed) / 1000000.0f;
    d->last_read_time = now;
    
    // Ограничение dt для стабильности фильтра
    if (d->dt > 0.1f) d->dt = 0.016f;
    if (d->dt < 0.001f) d->dt = 0.016f;
    
    readRawData();
    
    // Применение данных к комплементарному фильтру
    m_filter.setAccelData(m_ax, m_ay, m_az);
    m_filter.setGyroData(m_gx, m_gy, m_gz);
    m_filter.process(d->dt);
    
    m_ready = true;
}

float Mpu6050Imu::yaw() const {
    if (!m_ready) return m_yaw_offset;
    return m_filter.yaw() + m_yaw_offset;
}

float Mpu6050Imu::pitch() const {
    if (!m_ready) return 0.0f;
    return m_filter.pitch();
}

float Mpu6050Imu::roll() const {
    if (!m_ready) return 0.0f;
    return m_filter.roll();
}

void Mpu6050Imu::setYawOffset(float offset) {
    m_yaw_offset = offset;
}

} // namespace robo_chassis
