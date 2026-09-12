cmake_minimum_required(VERSION 3.16)
project(robo_chassis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Опция для сборки тестов
option(BUILD_TESTS "Build unit tests with Google Test" ON)

# Оптимизация для Raspberry Pi 2 (ARM Cortex-A7)
# Используем универсальные флаги, совместимые с x86 и ARM
if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l|aarch64")
    # Оптимизация специально для Cortex-A7 (Raspberry Pi 2)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -ffast-math")
    # Дополнительная оптимизация размера кода для экономии памяти
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fdata-sections -ffunction-sections")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()
set(CMAKE_BUILD_TYPE Release)

# Флаги линковщика для оптимизации размера
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections")

# Основной исполняемый файл (чистый C++ без Qt)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

add_executable(robo_chassis
    src/core/main.cpp
    src/core/tcp_server.cpp
    src/core/robot_logic.cpp
    src/core/exchangers/i2c_master.cpp
    src/core/exchangers/i2c_simulator.cpp
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
    src/core/sensors/sensor_fusion.cpp
    src/core/autonomy/autonomy_manager.cpp
    src/core/safety/safety_manager.cpp
)

# Поиск pthread
find_package(Threads REQUIRED)
target_link_libraries(robo_chassis PRIVATE Threads::Threads fmt)

# Поиск nlohmann/json
find_package(nlohmann_json REQUIRED)
if(nlohmann_json_FOUND)
    target_link_libraries(robo_chassis PRIVATE nlohmann_json::nlohmann_json)
    message(STATUS "nlohmann/json found: ${nlohmann_json_VERSION}")
else()
    message(FATAL_ERROR "nlohmann/json not found, please install nlohmann-json3-dev")
endif()

# Поиск wslay для WebSocket
find_package(wslay QUIET)
if(wslay_FOUND AND TARGET wslay::wslay)
    include_directories(${wslay_INCLUDE_DIRS})
    target_link_libraries(robo_chassis PRIVATE wslay::wslay)
    message(STATUS "wslay found via cmake")
else()
    # Fallback к pkg-config если cmake конфиг не найден
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WSLAY libwslay)
    if(WSLAY_FOUND)
        include_directories(${WSLAY_INCLUDE_DIRS})
        target_link_libraries(robo_chassis PRIVATE ${WSLAY_LIBRARIES})
        message(STATUS "wslay found via pkg-config: ${WSLAY_VERSION}")
    else()
        # Прямое указание путей
        target_include_directories(robo_chassis PRIVATE /usr/include)
        target_link_libraries(robo_chassis PRIVATE wslay)
        message(STATUS "wslay linked directly")
    endif()
endif()

# OpenSSL для SHA-1 в WebSocket handshake
find_package(OpenSSL REQUIRED)
target_link_libraries(robo_chassis PRIVATE OpenSSL::SSL OpenSSL::Crypto)
message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")

# pigpio для работы с GPIO (ультразвук) - замена wiringPi
find_library(PIGPIO_LIB NAMES pigpio)
if(PIGPIO_LIB)
    target_link_libraries(robo_chassis PRIVATE ${PIGPIO_LIB})
    target_compile_definitions(robo_chassis PRIVATE HAVE_PIGPIO)
    message(STATUS "pigpio library found: ${PIGPIO_LIB}")
else()
    # Не останавливаем сборку, но предупреждаем что ультразвук не будет работать
    message(WARNING "pigpio library not found. Ultrasonic sensor will not work. Install with: sudo apt-get install pigpio")
endif()

# Сборка тестов
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Установка
install(TARGETS robo_chassis DESTINATION bin)
