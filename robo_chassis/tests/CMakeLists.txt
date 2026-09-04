cmake_minimum_required(VERSION 3.10)
project(robo_chassis_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Find Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/../src/core
    ${PROJECT_SOURCE_DIR}/../src/drivers
    ${PROJECT_SOURCE_DIR}/../src/core/config
    ${PROJECT_SOURCE_DIR}/../src/core/logger
    ${PROJECT_SOURCE_DIR}/../src/core/autonomy
)

# Test executable
add_executable(run_tests
    test_pid.cpp
    test_sensor_fusion.cpp
    test_sensor_fusion_mock.cpp
)

target_link_libraries(run_tests
    GTest::gtest_main
    pthread
)

# Add tests
add_test(NAME PIDControllerTests COMMAND run_tests --gtest_filter="PIDControllerTest.*")
add_test(NAME SensorFusionTests COMMAND run_tests --gtest_filter="SensorFusionTest.*")
add_test(NAME SensorFusionMockTests COMMAND run_tests --gtest_filter="SensorFusionMockTest.*")
