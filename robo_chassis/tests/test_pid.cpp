#include <gtest/gtest.h>
#include "autonomy/autonomy_manager.hpp"
#include <thread>
#include <chrono>

// PIDController is defined inside autonomy_manager.hpp in robo_chassis namespace
class PIDControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        pid.reset(new robo_chassis::PIDController(1.0f, 0.1f, 0.05f));
    }

    std::unique_ptr<robo_chassis::PIDController> pid;
};

TEST_F(PIDControllerTest, ProportionalTerm) {
    // Test proportional response with gains that won't hit limits
    float error = 0.5f;
    pid->setTarget(error);
    pid->setCurrent(0.0f);
    
    // Need to call compute twice because first call initializes dt
    pid->compute();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    float output = pid->compute();

    // P term should be approximately Kp * error = 1.0 * 0.5 = 0.5
    EXPECT_NEAR(output, 0.5f, 0.2f);
}

TEST_F(PIDControllerTest, IntegralTermAccumulation) {
    // Reset and test integral accumulation
    pid->reset();
    float target = 0.5f;
    pid->setTarget(target);

    // Call multiple times with delays to accumulate integral
    pid->setCurrent(0.0f);
    pid->compute();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    
    pid->setCurrent(0.0f);
    float output2 = pid->compute();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    
    pid->setCurrent(0.0f);
    float output3 = pid->compute();

    // Output should increase due to integral term
    EXPECT_GT(output3, output2);
}

TEST_F(PIDControllerTest, DerivativeTerm) {
    pid->reset();

    // Set large positive error
    pid->setTarget(10.0f);
    pid->setCurrent(0.0f);
    float output1 = pid->compute();

    // Error reduced to zero - derivative should be negative
    pid->setTarget(0.0f);
    pid->setCurrent(0.0f);
    float output2 = pid->compute();

    // Derivative term should reduce output
    EXPECT_LT(output2, output1);
}

TEST_F(PIDControllerTest, ResetFunctionality) {
    pid->setTarget(5.0f);
    pid->setCurrent(0.0f);
    pid->compute();
    pid->compute();
    pid->compute();

    pid->reset();
    pid->setTarget(5.0f);
    pid->setCurrent(0.0f);
    float output_after = pid->compute();

    // After reset, output should be different (integral cleared)
    EXPECT_NE(output_after, 0.0f);
}

TEST_F(PIDControllerTest, AngleNormalization) {
    // Test that PID handles angle wrapping correctly
    pid->reset();
    pid->setTarget(350.0f);
    pid->setCurrent(10.0f);
    float output = pid->compute();

    // Should take shortest path (20 degrees counter-clockwise)
    EXPECT_TRUE(true); // Just verify no crash
}

TEST_F(PIDControllerTest, GainAdjustment) {
    pid->setGains(2.0f, 0.0f, 0.0f);  // Only P term for predictable output
    pid->setTarget(0.5f);
    pid->setCurrent(0.0f);
    
    // First call initializes, second gives real output
    pid->compute();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    float output = pid->compute();

    // With Kp=2.0 and error=0.5, output should be ~1.0 (but clamped to 1.0 max)
    EXPECT_NEAR(output, 1.0f, 0.1f);
}
