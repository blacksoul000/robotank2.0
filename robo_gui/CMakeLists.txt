cmake_minimum_required(VERSION 3.14)
project(robo_gui VERSION 0.1 LANGUAGES CXX)

add_definitions(-std=c++11 -fpic)
SET(CMAKE_BUILD_TYPE Debug)

option(GSTREAMER_STATIC "Link gstreamer as static libraries" OFF)
set(ANDROID_PACKAGE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/android CACHE INTERNAL "")

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake/)
include(RecurseDirs)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick Svg Network Bluetooth)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0
                                     gstreamer-base-1.0
                                     gstreamer-plugins-base-1.0
                                     gstreamer-app-1.0
                                     gstreamer-pbutils-1.0
                                     gstreamer-video-1.0
                                     gmodule-2.0
                                     glib-2.0)

set(CMAKE_AUTOMOC ON)

if (GSTREAMER_STATIC)
    message("* Building GStreamer static *")
    add_definitions(-DGSTREAMER_STATIC)
    pkg_check_modules(GSTGL REQUIRED gstreamer-gl-1.0 gstreamer-audio-1.0)
    pkg_check_modules(GST_PLUGINS REQUIRED
                                       gstvideotestsrc
                                       gstcoreelements
                                       gstrtpmanager
                                       gstvideoparsersbad
                                       gstrtp
                                       gstlibav
                                       gstvideoconvert
                                       gstautoconvert
                                       gstrtsp
                                       gstplayback
                                       gstopengl
                                       gstqmlgl
                                       gstudp
                                   )

    set(LIBS ${GST_PLUGINS_STATIC_LDFLAGS} ${GSTGL_STATIC_LDFLAGS} EGL)
else (GSTREAMER_STATIC)
    set(LIBS ${GSTREAMER_LDFLAGS})
endif()

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src src/*.cpp)

HEADER_DIRECTORIES(${CMAKE_SOURCE_DIR}/src DIRS)
include_directories(${DIRS} ${GSTREAMER_INCLUDE_DIRS})

file(GLOB_RECURSE QRC ${CMAKE_SOURCE_DIR}/src src/*.qrc)
qt5_add_resources(RESOURCES ${QRC})
set_property(SOURCE ${RESOURCES} PROPERTY SKIP_AUTOMOC ON)

if(ANDROID)
    set(LIBS ${LIBS} log)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${RESOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
endif()

target_compile_definitions(${PROJECT_NAME}
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER vitaliy.bondar.robotank
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

add_custom_target(mavlink mavgen.py --lang C --wire-protocol 1.0
    -o ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages
    ${CMAKE_SOURCE_DIR}/../common/mavlink/message_definitions/robotank.xml)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages
    ${CMAKE_CURRENT_BINARY_DIR}/mavlink_messages/robotank)

add_subdirectory(${CMAKE_SOURCE_DIR}/../common common)
target_link_libraries(${PROJECT_NAME} ${LIBS})
qt5_use_modules(${PROJECT_NAME} Core Qml Quick Svg Multimedia Network Bluetooth)
