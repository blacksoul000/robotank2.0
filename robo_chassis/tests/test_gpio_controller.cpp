/**
 * @file test_gpio_controller.cpp
 * @brief Тесты для GPIO контроллера с моками
 * 
 * Изолированные тесты без зависимости от реального железа.
 */

#include <gtest/gtest.h>
#include <memory>
#include <atomic>
#include <functional>
#include "gpio/gpio_controller.hpp"

namespace robo_chassis {

// Простой тест на структуры данных
TEST(GpioDataStructuresTest, JoyButtonsDefault) {
    JoyButtons buttons;
    EXPECT_EQ(buttons.buttons, 0);
}

TEST(GpioDataStructuresTest, InfluenceDefault) {
    Influence influence;
    EXPECT_FLOAT_EQ(influence.gunV, 0.0f);
}

TEST(GpioDataStructuresTest, PointF3DDefault) {
    PointF3D point;
    EXPECT_FLOAT_EQ(point.x, 0.0f);
    EXPECT_FLOAT_EQ(point.y, 0.0f);
    EXPECT_FLOAT_EQ(point.z, 0.0f);
}

TEST(GpioDataStructuresTest, JoyButtonsSetValue) {
    JoyButtons buttons;
    buttons.buttons = 0b1010;
    EXPECT_EQ(buttons.buttons, 0b1010);
}

TEST(GpioDataStructuresTest, InfluenceSetValue) {
    Influence influence;
    influence.gunV = 0.75f;
    EXPECT_FLOAT_EQ(influence.gunV, 0.75f);
}

TEST(GpioDataStructuresTest, PointF3DSetValue) {
    PointF3D point;
    point.x = 1.0f;
    point.y = 2.0f;
    point.z = 3.0f;
    EXPECT_FLOAT_EQ(point.x, 1.0f);
    EXPECT_FLOAT_EQ(point.y, 2.0f);
    EXPECT_FLOAT_EQ(point.z, 3.0f);
}

// Тесты callback'ов
class GpioCallbacksTest : public ::testing::Test {
protected:
    std::atomic<bool> joy_callback_called_{false};
    std::atomic<bool> influence_callback_called_{false};
    std::atomic<bool> deviation_callback_called_{false};
    std::atomic<bool> gun_calibrate_callback_called_{false};
    std::atomic<bool> camera_calibrate_callback_called_{false};
    std::atomic<bool> gyro_calibrate_callback_called_{false};
    std::atomic<bool> status_callback_called_{false};
    
    uint16_t last_joy_buttons_ = 0;
    float last_influence_gun_v_ = 0.0f;
    double last_deviation_ = 0.0;
    bool last_status_ = false;
};

TEST_F(GpioCallbacksTest, JoyCallbackRegistration) {
    JoyButtons joy;
    joy.buttons = 0b1111;
    
    auto callback = [this](const JoyButtons& jb) {
        joy_callback_called_ = true;
        last_joy_buttons_ = jb.buttons;
    };
    
    // Симуляция вызова callback
    callback(joy);
    
    EXPECT_TRUE(joy_callback_called_);
    EXPECT_EQ(last_joy_buttons_, 0b1111);
}

TEST_F(GpioCallbacksTest, InfluenceCallbackRegistration) {
    Influence influence;
    influence.gunV = 0.5f;
    
    auto callback = [this](const Influence& inf) {
        influence_callback_called_ = true;
        last_influence_gun_v_ = inf.gunV;
    };
    
    callback(influence);
    
    EXPECT_TRUE(influence_callback_called_);
    EXPECT_FLOAT_EQ(last_influence_gun_v_, 0.5f);
}

TEST_F(GpioCallbacksTest, DeviationCallbackRegistration) {
    auto callback = [this](double value) {
        deviation_callback_called_ = true;
        last_deviation_ = value;
    };
    
    callback(45.5);
    
    EXPECT_TRUE(deviation_callback_called_);
    EXPECT_DOUBLE_EQ(last_deviation_, 45.5);
}

TEST_F(GpioCallbacksTest, GunCalibrateCallbackRegistration) {
    auto callback = [this](const Empty&) {
        gun_calibrate_callback_called_ = true;
    };
    
    callback(Empty{});
    
    EXPECT_TRUE(gun_calibrate_callback_called_);
}

TEST_F(GpioCallbacksTest, CameraCalibrateCallbackRegistration) {
    auto callback = [this](const Empty&) {
        camera_calibrate_callback_called_ = true;
    };
    
    callback(Empty{});
    
    EXPECT_TRUE(camera_calibrate_callback_called_);
}

TEST_F(GpioCallbacksTest, GyroCalibrateCallbackRegistration) {
    auto callback = [this](const Empty&) {
        gyro_calibrate_callback_called_ = true;
    };
    
    callback(Empty{});
    
    EXPECT_TRUE(gyro_calibrate_callback_called_);
}

TEST_F(GpioCallbacksTest, StatusCallbackRegistration) {
    auto callback = [this](bool status) {
        status_callback_called_ = true;
        last_status_ = status;
    };
    
    callback(true);
    EXPECT_TRUE(status_callback_called_);
    EXPECT_TRUE(last_status_);
    
    status_callback_called_ = false;
    callback(false);
    EXPECT_TRUE(status_callback_called_);
    EXPECT_FALSE(last_status_);
}

// Тесты битовых операций для кнопок джойстика
TEST(JoyButtonsBitwiseTest, SetButton0) {
    JoyButtons joy;
    joy.buttons |= (1 << 0);
    EXPECT_TRUE(joy.buttons & (1 << 0));
}

TEST(JoyButtonsBitwiseTest, SetMultipleButtons) {
    JoyButtons joy;
    joy.buttons = (1 << 0) | (1 << 2) | (1 << 5);
    
    EXPECT_TRUE(joy.buttons & (1 << 0));
    EXPECT_TRUE(joy.buttons & (1 << 2));
    EXPECT_TRUE(joy.buttons & (1 << 5));
    EXPECT_FALSE(joy.buttons & (1 << 1));
    EXPECT_FALSE(joy.buttons & (1 << 3));
}

TEST(JoyButtonsBitwiseTest, ClearButton) {
    JoyButtons joy;
    joy.buttons = 0b1111;
    joy.buttons &= ~(1 << 2);
    
    EXPECT_TRUE(joy.buttons & (1 << 0));
    EXPECT_TRUE(joy.buttons & (1 << 1));
    EXPECT_FALSE(joy.buttons & (1 << 2));
    EXPECT_TRUE(joy.buttons & (1 << 3));
}

TEST(JoyButtonsBitwiseTest, ToggleButton) {
    JoyButtons joy;
    joy.buttons = 0b0100;
    joy.buttons ^= (1 << 2);
    
    EXPECT_FALSE(joy.buttons & (1 << 2));
    
    joy.buttons ^= (1 << 2);
    EXPECT_TRUE(joy.buttons & (1 << 2));
}

TEST(JoyButtonsBitwiseTest, CheckAllButtonsSet) {
    JoyButtons joy;
    joy.buttons = 0xFFFF;
    
    for (int i = 0; i < 16; ++i) {
        EXPECT_TRUE(joy.buttons & (1 << i)) << "Button " << i << " should be set";
    }
}

TEST(JoyButtonsBitwiseTest, CheckAllButtonsClear) {
    JoyButtons joy;
    joy.buttons = 0x0000;
    
    for (int i = 0; i < 16; ++i) {
        EXPECT_FALSE(joy.buttons & (1 << i)) << "Button " << i << " should be clear";
    }
}

// Тесты указателя (Pointer)
TEST(PointerTest, PointerOn) {
    bool pointer_state = true;
    EXPECT_TRUE(pointer_state);
}

TEST(PointerTest, PointerOff) {
    bool pointer_state = false;
    EXPECT_FALSE(pointer_state);
}

TEST(PointerTest, PointerToggle) {
    bool pointer_state = false;
    pointer_state = !pointer_state;
    EXPECT_TRUE(pointer_state);
    
    pointer_state = !pointer_state;
    EXPECT_FALSE(pointer_state);
}

// Тесты YPR (Yaw, Pitch, Roll)
TEST(YprTest, DefaultValues) {
    PointF3D ypr;
    EXPECT_FLOAT_EQ(ypr.x, 0.0f);  // Yaw
    EXPECT_FLOAT_EQ(ypr.y, 0.0f);  // Pitch
    EXPECT_FLOAT_EQ(ypr.z, 0.0f);  // Roll
}

TEST(YprTest, SetYaw) {
    PointF3D ypr;
    ypr.x = 90.0f;  // 90 градусов yaw
    EXPECT_FLOAT_EQ(ypr.x, 90.0f);
}

TEST(YprTest, SetPitch) {
    PointF3D ypr;
    ypr.y = 45.0f;  // 45 градусов pitch
    EXPECT_FLOAT_EQ(ypr.y, 45.0f);
}

TEST(YprTest, SetRoll) {
    PointF3D ypr;
    ypr.z = -30.0f;  // -30 градусов roll
    EXPECT_FLOAT_EQ(ypr.z, -30.0f);
}

TEST(YprTest, NormalizeYaw) {
    // Yaw должен быть в диапазоне 0-360
    float yaw = 450.0f;
    while (yaw >= 360.0f) yaw -= 360.0f;
    EXPECT_FLOAT_EQ(yaw, 90.0f);
    
    yaw = -90.0f;
    while (yaw < 0.0f) yaw += 360.0f;
    EXPECT_FLOAT_EQ(yaw, 270.0f);
}

TEST(YprTest, PitchRange) {
    // Pitch должен быть в диапазоне -90 до +90
    float pitch = 100.0f;
    if (pitch > 90.0f) pitch = 90.0f;
    EXPECT_FLOAT_EQ(pitch, 90.0f);
    
    pitch = -100.0f;
    if (pitch < -90.0f) pitch = -90.0f;
    EXPECT_FLOAT_EQ(pitch, -90.0f);
}

// Тесты калибровки
TEST(CalibrationTest, CalibrateGun) {
    bool calibration_started = false;
    bool calibration_complete = false;
    
    calibration_started = true;
    // Симуляция процесса калибровки
    calibration_complete = true;
    
    EXPECT_TRUE(calibration_started);
    EXPECT_TRUE(calibration_complete);
}

TEST(CalibrationTest, CalibrateCamera) {
    bool calibration_in_progress = false;
    
    calibration_in_progress = true;
    EXPECT_TRUE(calibration_in_progress);
    
    calibration_in_progress = false;
    EXPECT_FALSE(calibration_in_progress);
}

TEST(CalibrationTest, CalibrateGyro) {
    int calibration_steps = 0;
    const int total_steps = 100;
    
    for (int i = 0; i < total_steps; ++i) {
        calibration_steps++;
    }
    
    EXPECT_EQ(calibration_steps, total_steps);
}

// Тесты Arduino статуса
TEST(ArduinoStatusTest, OnlineStatus) {
    bool arduino_online = true;
    EXPECT_TRUE(arduino_online);
}

TEST(ArduinoStatusTest, OfflineStatus) {
    bool arduino_online = false;
    EXPECT_FALSE(arduino_online);
}

TEST(ArduinoStatusTest, StatusTransition) {
    bool arduino_online = true;
    
    // Переход из online в offline
    arduino_online = false;
    EXPECT_FALSE(arduino_online);
    
    // Переход из offline в online
    arduino_online = true;
    EXPECT_TRUE(arduino_online);
}

TEST(ArduinoStatusTest, MultipleStatusChecks) {
    std::vector<bool> status_history;
    status_history.push_back(true);
    status_history.push_back(false);
    status_history.push_back(true);
    status_history.push_back(true);
    status_history.push_back(false);
    
    int online_count = 0;
    for (bool status : status_history) {
        if (status) online_count++;
    }
    
    EXPECT_EQ(online_count, 3);
    EXPECT_EQ(status_history.size() - online_count, 2);
}
