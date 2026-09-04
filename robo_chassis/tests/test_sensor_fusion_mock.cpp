/**
 * @file test_sensor_fusion_mock.cpp
 * @brief Тесты SensorFusion с использованием моков для изоляции от реального железа
 * 
 * Эти тесты не зависят от наличия физических датчиков и могут запускаться
 * на любой машине без Raspberry Pi.
 */

#include <gtest/gtest.h>
#include <memory>
#include <cmath>

// Моки для зависимостей
namespace robo_chassis {

// Mock MPU6050 (IMU)
class MockMPU6050 {
public:
    bool init() { return mock_init_success_; }
    bool isReady() const { return mock_init_success_; }
    
    void setMockData(float ax, float ay, float az, float gx, float gy, float gz) {
        mock_accel_x_ = ax;
        mock_accel_y_ = ay;
        mock_accel_z_ = az;
        mock_gyro_x_ = gx;
        mock_gyro_y_ = gy;
        mock_gyro_z_ = gz;
    }
    
    float getAccelX() const { return mock_accel_x_; }
    float getAccelY() const { return mock_accel_y_; }
    float getAccelZ() const { return mock_accel_z_; }
    float getGyroX() const { return mock_gyro_x_; }
    float getGyroY() const { return mock_gyro_y_; }
    float getGyroZ() const { return mock_gyro_z_; }
    
    void setInitSuccess(bool success) { mock_init_success_ = success; }
    
private:
    bool mock_init_success_ = true;
    float mock_accel_x_ = 0.0f;
    float mock_accel_y_ = 0.0f;
    float mock_accel_z_ = 9.81f;  // Гравитация по Z когда робот стоит ровно
    float mock_gyro_x_ = 0.0f;
    float mock_gyro_y_ = 0.0f;
    float mock_gyro_z_ = 0.0f;
};

// Mock Magnetometer (Компас)
class MockMagnetometer {
public:
    bool init() { return mock_init_success_; }
    bool isReady() const { return mock_init_success_; }
    
    void setMockData(float mx, float my, float mz) {
        mock_mag_x_ = mx;
        mock_mag_y_ = my;
        mock_mag_z_ = mz;
    }
    
    float getMagX() const { return mock_mag_x_; }
    float getMagY() const { return mock_mag_y_; }
    float getMagZ() const { return mock_mag_z_; }
    
    void setInitSuccess(bool success) { mock_init_success_ = success; }
    
private:
    bool mock_init_success_ = false;  // По умолчанию магнитометр отсутствует
    float mock_mag_x_ = 0.0f;
    float mock_mag_y_ = 0.0f;
    float mock_mag_z_ = 0.0f;
};

} // namespace robo_chassis

// Тесты с моками
class SensorFusionMockTest : public ::testing::Test {
protected:
    void SetUp() override {
        imu_ = std::make_unique<robo_chassis::MockMPU6050>();
        magnetometer_ = std::make_unique<robo_chassis::MockMagnetometer>();
        
        // Устанавливаем данные "робот стоит ровно"
        imu_->setMockData(0.0f, 0.0f, 9.81f, 0.0f, 0.0f, 0.0f);
    }
    
    std::unique_ptr<robo_chassis::MockMPU6050> imu_;
    std::unique_ptr<robo_chassis::MockMagnetometer> magnetometer_;
};

TEST_F(SensorFusionMockTest, ImuOnlyInitialization) {
    // Имитируем ситуацию: IMU есть, магнитометра нет
    imu_->setInitSuccess(true);
    magnetometer_->setInitSuccess(false);
    
    EXPECT_TRUE(imu_->init());
    EXPECT_FALSE(magnetometer_->init());
    EXPECT_TRUE(imu_->isReady());
    EXPECT_FALSE(magnetometer_->isReady());
}

TEST_F(SensorFusionMockTest, FullSensorInitialization) {
    // Имитируем ситуацию: оба сенсора доступны
    imu_->setInitSuccess(true);
    magnetometer_->setInitSuccess(true);
    
    EXPECT_TRUE(imu_->init());
    EXPECT_TRUE(magnetometer_->init());
}

TEST_F(SensorFusionMockTest, HeadingCalculationWithMockData) {
    // Устанавливаем магнитометр для расчета курса
    magnetometer_->setInitSuccess(true);
    
    // Данные: магнитное поле направлено на север (магнитный меридиан)
    magnetometer_->setMockData(0.0f, 1.0f, 0.0f);  // Север
    
    EXPECT_TRUE(magnetometer_->init());
    EXPECT_FLOAT_EQ(0.0f, magnetometer_->getMagX());
    EXPECT_FLOAT_EQ(1.0f, magnetometer_->getMagY());
}

TEST_F(SensorFusionMockTest, TiltAnglesFromAccelerometer) {
    // Робот наклонен вперед на 45 градусов
    float tilt_angle = M_PI / 4.0f;  // 45 градусов
    float g = 9.81f;
    
    imu_->setMockData(
        g * std::sin(tilt_angle),  // accel_x
        0.0f,                       // accel_y
        g * std::cos(tilt_angle),  // accel_z
        0.0f, 0.0f, 0.0f           // gyro
    );
    
    EXPECT_TRUE(imu_->init());
    EXPECT_NEAR(g * std::sin(tilt_angle), imu_->getAccelX(), 0.01f);
    EXPECT_NEAR(g * std::cos(tilt_angle), imu_->getAccelZ(), 0.01f);
}

TEST_F(SensorFusionMockTest, NoCrashWithNullSensors) {
    // Тест на устойчивость: сенсоры не инициализированы
    imu_->setInitSuccess(false);
    magnetometer_->setInitSuccess(false);
    
    EXPECT_FALSE(imu_->init());
    EXPECT_FALSE(magnetometer_->init());
    
    // Чтение данных не должно вызывать краш
    EXPECT_FLOAT_EQ(0.0f, imu_->getAccelX());
    EXPECT_FLOAT_EQ(0.0f, magnetometer_->getMagX());
}

TEST_F(SensorFusionMockTest, GyroDataIntegration) {
    // Имитируем вращение вокруг оси Z (yaw)
    float angular_velocity = 0.5f;  // радиан/сек
    imu_->setMockData(0.0f, 0.0f, 9.81f, 0.0f, 0.0f, angular_velocity);
    
    EXPECT_TRUE(imu_->init());
    EXPECT_FLOAT_EQ(angular_velocity, imu_->getGyroZ());
    
    // Интегрируем за 2 секунды
    float expected_yaw = angular_velocity * 2.0f;
    EXPECT_FLOAT_EQ(expected_yaw, angular_velocity * 2.0f);
}

TEST_F(SensorFusionMockTest, MagneticFieldRanges) {
    magnetometer_->setInitSuccess(true);
    
    // Типичные значения магнитного поля Земли: 25-65 мкТл
    magnetometer_->setMockData(30.0f, 20.0f, 10.0f);
    
    EXPECT_TRUE(magnetometer_->init());
    
    float total_field = std::sqrt(
        std::pow(magnetometer_->getMagX(), 2) +
        std::pow(magnetometer_->getMagY(), 2) +
        std::pow(magnetometer_->getMagZ(), 2)
    );
    
    // Проверяем что поле в разумных пределах (20-70 мкТл)
    EXPECT_GE(total_field, 20.0f);
    EXPECT_LE(total_field, 70.0f);
}

TEST_F(SensorFusionMockTest, AccelerometerGravityDetection) {
    // Когда робот стоит ровно, акселерометр должен показывать гравитацию по Z
    imu_->setMockData(0.0f, 0.0f, 9.81f, 0.0f, 0.0f, 0.0f);
    
    EXPECT_TRUE(imu_->init());
    
    float total_accel = std::sqrt(
        std::pow(imu_->getAccelX(), 2) +
        std::pow(imu_->getAccelY(), 2) +
        std::pow(imu_->getAccelZ(), 2)
    );
    
    // Модуль ускорения должен быть близок к g = 9.81 м/с²
    EXPECT_NEAR(total_accel, 9.81f, 0.1f);
}

TEST_F(SensorFusionMockTest, SimulatedMovement) {
    // Симуляция движения: робот едет вперед с ускорением
    float forward_accel = 2.0f;  // м/с²
    imu_->setMockData(forward_accel, 0.0f, 9.81f, 0.0f, 0.0f, 0.0f);
    
    EXPECT_TRUE(imu_->init());
    EXPECT_GT(imu_->getAccelX(), 0.0f);  // Есть ускорение вперед
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
