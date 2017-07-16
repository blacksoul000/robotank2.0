cmake_minimum_required(VERSION 2.8.3)
project(robo_chassis)

add_definitions(-std=c++11)
option(PICAM "Enables RaspberryPi Camera" OFF)
set (CMAKE_BUILD_TYPE Debug)

set (OpenTLD_DIR ${CMAKE_SOURCE_DIR}/3rdparty/OpenTLD)

include(ExternalProject)
ExternalProject_Add(OpenTLD
                    SOURCE_DIR ${OpenTLD_DIR}
                    PREFIX ${CMAKE_BINARY_DIR}/OpenTLD
                    BINARY_DIR ${CMAKE_BINARY_DIR}/OpenTLD
                    CMAKE_ARGS -DWITH_OPENMP=ON
                    INSTALL_COMMAND "")

#include(${CMAKE_SOURCE_DIR}/../cmake/FindOpenCV.cmake)
include(${CMAKE_SOURCE_DIR}/../cmake/RecurseDirs.cmake)

find_package(OpenCV 3)

find_package(Qt5Core)
find_package(Qt5Network)
find_package(Qt5SerialPort)
find_package(Qt5Bluetooth)

include(FindPkgConfig)
pkg_check_modules(GSTRTSP gstreamer-rtsp-server-1.0)

set(LIBS
    Qt5::Core
    Qt5::Network
    Qt5::SerialPort
    Qt5::Bluetooth
    ${OpenCV_LIBS}
    opencv_tracking
    ${CMAKE_BINARY_DIR}/OpenTLD/lib/libopentld.a
    ${CMAKE_BINARY_DIR}/OpenTLD/lib/libcvblobs.a
    gomp
    wiringPi
    gstrtsp-1.0
    gstapp-1.0
    ${GSTRTSP_LIBRARIES}
)

if(PICAM)
    message("** Enabling RPi Camera **")
    add_definitions(-DPICAM)
endif(PICAM)

set(CMAKE_AUTOMOC ON)

HEADER_DIRECTORIES(src DIRS)
include_directories(${CMAKE_SOURCE_DIR}/../common)
include_directories(${DIRS})
include_directories(${OpenTLD_DIR}/src)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${GSTRTSP_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES src src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
add_dependencies(${PROJECT_NAME} OpenTLD)

target_link_libraries(${PROJECT_NAME} ${LIBS})
