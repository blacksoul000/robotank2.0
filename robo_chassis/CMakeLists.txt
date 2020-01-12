cmake_minimum_required(VERSION 2.8.3)
project(robo_chassis)

add_definitions(-std=c++11 -Wall)
option(PICAM "Enables RaspberryPi Camera" OFF)
set (CMAKE_BUILD_TYPE Debug)

set (OpenTLD_DIR ${CMAKE_SOURCE_DIR}/3rdparty/OpenTLD)

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

include(ExternalProject)
ExternalProject_Add(OpenTLD
                    SOURCE_DIR ${OpenTLD_DIR}
                    PREFIX ${CMAKE_BINARY_DIR}/OpenTLD
                    BINARY_DIR ${CMAKE_BINARY_DIR}/OpenTLD
                    CMAKE_ARGS -DWITH_OPENMP=ON
                    INSTALL_COMMAND "")

include(RecurseDirs)

find_package(OpenCV COMPONENTS core imgproc video OPTIONAL_COMPONENTS tracking)
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
    ${OpenCV_LIBS}
    ${CMAKE_BINARY_DIR}/OpenTLD/lib/libopentld.a
    ${CMAKE_BINARY_DIR}/OpenTLD/lib/libcvblobs.a
    gomp
)

if (${OPENCV_TRACKING_FOUND})
    set(LIBS ${LIBS} opencv_tracking)
    add_definitions(-DWITH_OPENCV_TRACKING)
else (${OPENCV_TRACKING_FOUND})
    set(FEATURES ${FEATURES} "tracking")
endif(${OPENCV_TRACKING_FOUND})

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
include_directories(${OpenTLD_DIR}/src)
include_directories(${OpenCV_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES src src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

add_custom_target(mavlink mavgen.py --lang C --wire-protocol 1.0 
    -o ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages
    ${CMAKE_SOURCE_DIR}/../common/mavlink/message_definitions/robotank.xml)
add_dependencies(${PROJECT_NAME} OpenTLD)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages
    ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages/robotank)

add_subdirectory(${CMAKE_SOURCE_DIR}/../common common)

target_link_libraries(${PROJECT_NAME} ${LIBS})

if (FEATURES)
    message (WARNING "DISABLED FEATURES:" "${FEATURES}")
endif (FEATURES)
