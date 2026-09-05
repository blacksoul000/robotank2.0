cmake_minimum_required(VERSION 3.10)
project(robo_chassis_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/../src/core
    ${PROJECT_SOURCE_DIR}/../src/drivers
    ${PROJECT_SOURCE_DIR}/../src/core/config
    ${PROJECT_SOURCE_DIR}/../src/core/logger
    ${PROJECT_SOURCE_DIR}/../src/core/autonomy
    ${PROJECT_SOURCE_DIR}/../src/core/sensors
    ${PROJECT_SOURCE_DIR}/../src/core/gpio
)

# Source files needed for tests
set(TEST_SOURCES
    test_pid.cpp
    test_sensor_fusion.cpp
    test_sensor_fusion_mock.cpp
    ../src/core/autonomy/autonomy_manager.cpp
    ../src/core/sensors/sensor_fusion.cpp
    ../src/core/sensors/compass_ultrasonic.cpp
    ../src/core/gpio/complementary_filter.cpp
    ../src/core/gpio/mpu6050_imu.cpp
    ../src/core/gpio/gpio_controller.cpp
    ../src/core/config/config.cpp
    ../src/core/logger/logger.cpp
)

# Test executable
add_executable(run_tests ${TEST_SOURCES})

target_link_libraries(run_tests
    GTest::gtest_main
    pthread
    Threads::Threads
)

# Add tests
add_test(NAME PIDControllerTests COMMAND run_tests --gtest_filter="PIDControllerTest.*")
add_test(NAME SensorFusionTests COMMAND run_tests --gtest_filter="SensorFusionTest.*")
add_test(NAME SensorFusionMockTests COMMAND run_tests --gtest_filter="SensorFusionMockTest.*")
