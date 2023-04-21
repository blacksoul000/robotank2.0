cmake_minimum_required(VERSION 2.8.3)
project(robo_chassis)

add_definitions(-std=c++11 -Wall)
option(PICAM "Enables RaspberryPi Camera" OFF)
option(WITH_TRACKING "Enables tracking algo" OFF)
set (CMAKE_BUILD_TYPE Debug)

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

include(RecurseDirs)

find_package(pigpio)

find_package(Qt5Core)
find_package(Qt5Network)
find_package(Qt5SerialPort)
find_package(Qt5Bluetooth)

include(FindPkgConfig)
pkg_check_modules(GSTRTSP gstreamer-rtsp-server-1.0)
pkg_check_modules(GST gstreamer-1.0)

set(LIBS
    Qt5::Core
    Qt5::Network
    Qt5::SerialPort
    Qt5::Bluetooth
)

if(WITH_TRACKING)
    message("** Enabling Tracking **")
    add_definitions(-DTRACKING)

    set (OpenTLD_DIR ${CMAKE_SOURCE_DIR}/3rdparty/OpenTLD)
    include(ExternalProject)
    ExternalProject_Add(OpenTLD
            SOURCE_DIR ${OpenTLD_DIR}
            PREFIX ${CMAKE_BINARY_DIR}/OpenTLD
            BINARY_DIR ${CMAKE_BINARY_DIR}/OpenTLD
            CMAKE_ARGS -DWITH_OPENMP=ON
            INSTALL_COMMAND "")

    find_package(OpenCV COMPONENTS core imgproc video OPTIONAL_COMPONENTS tracking)

    set (LIBS ${LIBS}
        ${OpenCV_LIBS}
        ${CMAKE_BINARY_DIR}/OpenTLD/lib/libopentld.a
        ${CMAKE_BINARY_DIR}/OpenTLD/lib/libcvblobs.a
        gomp
    )

    if (${OPENCV_TRACKING_FOUND})
        set(LIBS ${LIBS} opencv_tracking)
        add_definitions(-DWITH_OPENCV_TRACKING)
    else (${OPENCV_TRACKING_FOUND})
        set(FEATURES ${FEATURES} "OpenCV tracking")
    endif(${OPENCV_TRACKING_FOUND})

    include_directories(${OpenTLD_DIR}/src)
    include_directories(${OpenCV_INCLUDE_DIRS})
else(WITH_TRACKING)
    set(FEATURES ${FEATURES} "Tracking")
endif(WITH_TRACKING)

if (${PIGPIO_FOUND})
    set(LIBS ${LIBS} pigpio)
    add_definitions(-DWITH_GPIO)
else (${PIGPIO_FOUND})
    set (FEATURES ${FEATURES} "GPIO control")
endif(${PIGPIO_FOUND})

if (${GST_FOUND})
    set(LIBS ${LIBS} ${GST_LIBRARIES})
    add_definitions(-DWITH_GST)
    include_directories(${GST_INCLUDE_DIRS})
else (${GST_FOUND})
    set(FEATURES ${FEATURES} "FPV")
endif(${GST_FOUND})

if(PICAM)
    message("** Enabling RPi Camera **")
    add_definitions(-DPICAM)
endif(PICAM)

set(CMAKE_AUTOMOC ON)

HEADER_DIRECTORIES(src DIRS)
include_directories(${DIRS})

file(GLOB_RECURSE MAVLINK_SOURCES src src/tasks/mavlink_exchanger/*.cpp)
file(GLOB_RECURSE TRACKING_SOURCES src src/tasks/tracker/*.cpp)

set (SOURCES
    src/pub_sub/call_event.cpp
    src/pub_sub/pub_sub.cpp
    src/pub_sub/publisher.cpp
    src/pub_sub/subscriber.cpp
    src/task_manager/task_manager.cpp
    src/tasks/arduino_exchanger/arduino_exchanger.cpp
    src/tasks/arduino_exchanger/exchangers/i2c_master.cpp
    src/tasks/arduino_exchanger/exchangers/uart.cpp
    src/tasks/arduino_exchanger/exchangers/i_exchanger.cpp
    src/tasks/bluetooth_manager/bluetooth_manager.cpp
    src/tasks/config_handler/config_handler.cpp
    src/tasks/gamepad_controller/gamepad_controller.cpp
    src/tasks/robo_core/robo_core.cpp
    src/tasks/robo_core/pid/pid.cpp
    src/tasks/robo_core/pid/pid.cpp
    src/tasks/video_source/video_source.cpp
    src/tasks/video_source/rtsp_server/rtsp_server.cpp
    src/tasks/video_source/video_server/video_server.cpp
    src/thread_controller/task_worker.cpp
    src/thread_controller/thread_controller.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES} ${MAVLINK_SOURCES})

add_custom_target(mavlink mavgen.py --lang C --wire-protocol 1.0 
    -o ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages
    ${CMAKE_SOURCE_DIR}/../common/mavlink/message_definitions/robotank.xml)

if(WITH_TRACKING)
    target_sources(${PROJECT_NAME} PRIVATE ${TRACKING_SOURCES})
    add_dependencies(${PROJECT_NAME} OpenTLD)
endif(WITH_TRACKING)

if (${PIGPIO_FOUND})
    set (GPIO_SOURCES
            src/tasks/gpio_controller/gpio_controller.cpp
            src/tasks/gpio_controller/filter/complementary_filter.cpp
            src/tasks/gpio_controller/imu/mpu6050/mpu6050_dmp.cpp
            src/tasks/gpio_controller/imu/mpu6050/mpu6050_raw.cpp
            src/tasks/gpio_controller/imu/mpu6050/Impl/I2Cdev.cpp
            src/tasks/gpio_controller/imu/mpu6050/Impl/MPU6050.cpp
    )
    target_sources(${PROJECT_NAME} PRIVATE ${GPIO_SOURCES})
endif(${PIGPIO_FOUND})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages
    ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages/robotank)

add_subdirectory(${CMAKE_SOURCE_DIR}/../common common)

target_link_libraries(${PROJECT_NAME} ${LIBS})

if (FEATURES)
    message (WARNING "DISABLED FEATURES:" "${FEATURES}")
endif (FEATURES)
