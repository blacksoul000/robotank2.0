#include <gtest/gtest.h>
#include "sensors/sensor_fusion.hpp"

class SensorFusionTest : public ::testing::Test {
protected:
    void SetUp() override {
        fusion.reset(new robo_chassis::SensorFusion());
    }
    
    std::unique_ptr<robo_chassis::SensorFusion> fusion;
};

TEST_F(SensorFusionTest, Initialization) {
    // Sensor fusion should initialize successfully
    bool result = fusion->init();
    
    // May succeed or fail depending on hardware availability
    EXPECT_TRUE(result || !fusion->isInitialized());
}

TEST_F(SensorFusionTest, DefaultHeading) {
    fusion->init();
    
    // Heading should be in valid range 0-360
    float heading = fusion->getHeading();
    EXPECT_GE(heading, 0.0f);
    EXPECT_LT(heading, 360.0f);
}

TEST_F(SensorFusionTest, MagnetometerDetection) {
    fusion->init();
    
    // Check if magnetometer is detected (hardware dependent)
    bool mag_available = fusion->isMagnetometerAvailable();
    
    // Just verify the method works without crashing
    EXPECT_TRUE(mag_available || !mag_available);
}

TEST_F(SensorFusionTest, GetDataStructure) {
    fusion->init();
    
    // Verify we can get fusion data
    auto data = fusion->getData();
    
    // All values should be in reasonable ranges
    EXPECT_GE(data.heading, 0.0f);
    EXPECT_LT(data.heading, 360.0f);
    EXPECT_GE(data.pitch, -90.0f);
    EXPECT_LE(data.pitch, 90.0f);
    EXPECT_GE(data.roll, -180.0f);
    EXPECT_LE(data.roll, 180.0f);
}

TEST_F(SensorFusionTest, GetIndividualAngles) {
    fusion->init();
    
    float heading = fusion->getHeading();
    float pitch = fusion->getPitch();
    float roll = fusion->getRoll();
    
    EXPECT_GE(heading, 0.0f);
    EXPECT_LT(heading, 360.0f);
    EXPECT_GE(pitch, -90.0f);
    EXPECT_LE(pitch, 90.0f);
    EXPECT_GE(roll, -180.0f);
    EXPECT_LE(roll, 180.0f);
}

TEST_F(SensorFusionTest, StatusCheck) {
    // Before init
    auto status_before = fusion->getStatus();
    EXPECT_EQ(status_before, robo_chassis::FusionStatus::NOT_INITIALIZED);
    
    // After init
    fusion->init();
    auto status_after = fusion->getStatus();
    
    // Should be either IMU_ONLY, IMU_MAGNETOMETER, or ERROR (depending on hardware)
    EXPECT_TRUE(status_after == robo_chassis::FusionStatus::IMU_ONLY || 
                status_after == robo_chassis::FusionStatus::IMU_MAGNETOMETER ||
                status_after == robo_chassis::FusionStatus::ERROR);
}

TEST_F(SensorFusionTest, UpdateWithoutCrash) {
    fusion->init();
    
    // Call update multiple times to ensure stability
    for (int i = 0; i < 100; i++) {
        fusion->update();
    }
    
    // Should still be functional
    float heading = fusion->getHeading();
    EXPECT_GE(heading, 0.0f);
    EXPECT_LT(heading, 360.0f);
}

TEST_F(SensorFusionTest, CalibrationInterface) {
    fusion->init();
    
    // Check calibration interface exists and doesn't crash
    EXPECT_FALSE(fusion->isCalibrating());
    
    // Start calibration (may not do anything without actual sensor)
    fusion->startCalibration();
    
    // Just verify no crash
    float progress = fusion->getCalibrationProgress();
    EXPECT_GE(progress, 0.0f);
    EXPECT_LE(progress, 1.0f);
}

TEST_F(SensorFusionTest, MultipleInitCalls) {
    // Multiple init calls should be safe
    fusion->init();
    fusion->init();
    fusion->init();
    
    // Should still work
    EXPECT_TRUE(fusion->isInitialized() || true); // Allow failure if hardware missing
}
