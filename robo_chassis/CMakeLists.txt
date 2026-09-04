cmake_minimum_required(VERSION 3.16)
project(robo_chassis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Оптимизация для Raspberry Pi 2 (ARM Cortex-A7)
# Используем универсальные флаги, совместимые с x86 и ARM
if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l|aarch64")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()
set(CMAKE_BUILD_TYPE Release)

# Основной исполняемый файл (чистый C++ без Qt)
include_directories(${CMAKE_SOURCE_DIR}/src/core)

add_executable(robo_chassis
    src/core/main.cpp
    src/core/tcp_server.cpp
    src/core/serial_port.cpp
    src/core/robot_logic.cpp
    src/core/exchangers/i2c_master.cpp
    src/core/exchangers/uart_exchanger.cpp
    src/core/gpio/complementary_filter.cpp
    src/core/gpio/gpio_controller.cpp
    src/core/gpio/mpu6050_imu.cpp
    src/core/config/config.cpp
    src/core/logger/logger.cpp
    src/core/system_monitor/system_monitor.cpp
    src/core/memory/memory_manager.cpp
    src/core/websocket_server/websocket_server.cpp
    src/core/sensors/sensor_manager.cpp
    src/core/sensors/compass_ultrasonic.cpp
)

# Поиск pthread
find_package(Threads REQUIRED)
target_link_libraries(robo_chassis PRIVATE Threads::Threads)

# wiringPi для работы с GPIO (ультразвук)
find_library(WIRINGPI_LIB NAMES wiringPi)
if(WIRINGPI_LIB)
    target_link_libraries(robo_chassis PRIVATE ${WIRINGPI_LIB})
    message(STATUS "wiringPi library found")
else()
    message(WARNING "wiringPi library not found, ultrasonic sensor will not work")
endif()

# Опционально: pigpio для работы с GPIO
option(USE_PIGPIO "Use pigpio library for GPIO control" ON)
if(USE_PIGPIO)
    find_library(PIGPIO_LIB NAMES pigpio pigpiod_if2)
    if(PIGPIO_LIB)
        target_link_libraries(robo_chassis PRIVATE ${PIGPIO_LIB})
        target_compile_definitions(robo_chassis PRIVATE HAVE_PIGPIO)
        message(STATUS "pigpio library found: ${PIGPIO_LIB}")
    else()
        message(WARNING "pigpio library not found, GPIO functionality will be limited")
    endif()
endif()

# Установка
install(TARGETS robo_chassis DESTINATION bin)
