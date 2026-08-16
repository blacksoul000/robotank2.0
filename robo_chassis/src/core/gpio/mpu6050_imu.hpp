#ifndef MPU6050_IMU_HPP
#define MPU6050_IMU_HPP

#include "i_imu.hpp"
#include "complementary_filter.hpp"
#include <string>
#include <vector>
#include <memory>

namespace robo_chassis {

/**
 * @brief Реализация IMU на основе MPU6050
 * 
 * Использует I2C для чтения данных с акселерометра и гироскопа,
 * применяет комплементарный фильтр для вычисления углов ориентации.
 */
class Mpu6050Imu : public IImu {
public:
    /**
     * @brief Конструктор
     * @param i2c_device Путь к I2C устройству (например, "/dev/i2c-1")
     * @param address I2C адрес устройства (0x68 или 0x69)
     * @param yaw_offset Смещение угла yaw для калибровки
     */
    explicit Mpu6050Imu(
        const std::string& i2c_device = "/dev/i2c-1",
        int address = 0x68,
        float yaw_offset = 0.0f
    );
    
    ~Mpu6050Imu() override;
    
    bool init() override;
    bool isReady() const override;
    void readData() override;
    
    float yaw() const override;
    float pitch() const override;
    float roll() const override;
    
    /**
     * @brief Установить смещение yaw для калибровки
     */
    void setYawOffset(float offset);
    
    /**
     * @brief Получить текущее смещение yaw
     */
    float getYawOffset() const { return m_yaw_offset; }

private:
    class Impl;
    Impl* d;
    
    int m_fd;
    int m_address;
    std::string m_device;
    float m_yaw_offset;
    bool m_initialized;
    bool m_ready;
    
    ComplementaryFilter m_filter;
    
    // Данные сенсоров
    float m_ax, m_ay, m_az;  // Акселерометр
    float m_gx, m_gy, m_gz;  // Гироскоп
    
    bool openI2c();
    void closeI2c();
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t* data, size_t len);
    void readRawData();
    void calibrateGyro();
};

} // namespace robo_chassis

#endif // MPU6050_IMU_HPP
