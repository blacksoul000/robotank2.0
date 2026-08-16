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
add_executable(robo_chassis
    src/core/main.cpp
    src/core/tcp_server.cpp
    src/core/serial_port.cpp
    src/core/robot_logic.cpp
)

# Поиск pthread
find_package(Threads REQUIRED)
target_link_libraries(robo_chassis PRIVATE Threads::Threads)

# Установка
install(TARGETS robo_chassis DESTINATION bin)
