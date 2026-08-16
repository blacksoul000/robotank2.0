cmake_minimum_required(VERSION 3.10)
project(webrtc_streamer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)

# Находим Qt и GStreamer
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)

# Опция для включения камеры Raspberry Pi
option(PICAM "Use Raspberry Pi Camera" OFF)

if(PICAM)
    add_definitions(-DPICAM)
    message("** Enabling RPi Camera support **")
endif()

set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME} 
    main.cpp 
    webrtc_streamer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${GST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} 
    Qt5::Core
    Qt5::Network
    ${GST_LIBRARIES}
    pthread
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
