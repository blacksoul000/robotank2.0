cmake_minimum_required(VERSION 2.8.3)
project(robo_gui)

add_definitions(-std=c++11)
SET(CMAKE_BUILD_TYPE Debug)

if(ANDROID)
    set (ENV{JAVA_HOME} /usr/lib/jvm/java-7-openjdk-amd64)
    set (QT_ANDROID_ANT /usr/bin/ant)
    set (PYTHON_EXECUTABLE /usr/bin/python)
    set (ANDROID_COMPILER_VERSION 4.9)

    set (QTDIR /opt/Qt/5.5)
    set (Qt5Core_DIR ${QTDIR}/android_armv7/lib/cmake/Qt5Core)
    set (Qt5Gui_DIR ${QTDIR}/android_armv7/lib/cmake/Qt5Gui)
    set (Qt5Quick_DIR ${QTDIR}/android_armv7/lib/cmake/Qt5Quick)
    set (Qt5Svg_DIR ${QTDIR}/android_armv7/lib/cmake/Qt5Svg)
    set (Qt5Multimedia_DIR ${QTDIR}/android_armv7/lib/cmake/Qt5Multimedia)
    set (Qt5AndroidExtras_DIR ${QTDIR}/android_armv7/lib/cmake/Qt5AndroidExtras)
endif()

include(${CMAKE_SOURCE_DIR}/../cmake/RecurseDirs.cmake)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Qml)
find_package(Qt5Quick)
find_package(Qt5Multimedia)
find_package(Qt5Svg)
find_package(Qt5Network)

set(CMAKE_AUTOMOC ON)

set(LIBS
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::Multimedia
    Qt5::Svg
    Qt5::Network
    z
)

if (ANDROID)
    find_package(Qt5AndroidExtras)
    set (LIBS ${LIBS} Qt5::AndroidExtras log libjpeg libtiff libjasper libpng IlmImf android)
endif()

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src src/*.cpp)

HEADER_DIRECTORIES(${CMAKE_SOURCE_DIR}/src DIRS)
include_directories(${DIRS})
include_directories(${CMAKE_SOURCE_DIR}/../common)

qt5_add_resources(RESOURCES ${CMAKE_SOURCE_DIR}/src/qml.qrc
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/resources.qrc)

if(ANDROID)
    include_directories(${ANDROID_SYSROOT}/usr/include)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${RESOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION 0.1)
else(ANDROID)
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
endif()

target_link_libraries(${PROJECT_NAME} ${LIBS})

if(ANDROID)
    set (QT_ANDROID_APP_NAME ${PROJECT_NAME})
    include(3rdparty/qt-android-cmake/AddQtAndroidApk.cmake)
    add_qt_android_apk(${PROJECT_NAME}_apk.
                        ${PROJECT_NAME}
                        NAME "Robotank"
                        PACKAGE_NAME "vitaliy.bondar.robotank"
                        PACKAGE_SOURCES ${CMAKE_SOURCE_DIR}/android
)
endif()