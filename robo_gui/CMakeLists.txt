cmake_minimum_required(VERSION 2.8.3)
project(robo_gui)

add_definitions(-std=c++11 -fpic)
SET(CMAKE_BUILD_TYPE Debug)

if(ANDROID)
    set (QT_ANDROID_ANT /usr/bin/ant)
    set (PYTHON_EXECUTABLE /usr/bin/python)
    set (ANDROID_COMPILER_VERSION 4.9)
endif()

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

if (ANDROID)
    find_package(Qt5AndroidExtras)
    pkg_check_modules(GSTGL REQUIRED gstreamer-gl-1.0 gstreamer-audio-1.0)
    pkg_check_modules(GST_PLUGINS REQUIRED
                                       gstvideotestsrc
                                       gstcoreelements
                                       gstrtpmanager
                                       gstvideoparsersbad
                                       gstrtp
#                                        gstx264
# #                                        gstopenh264
                                       gstlibav
                                       gstvideoconvert
                                       gstautoconvert
                                       gstrtsp
                                       gstplayback
                                       gstopengl
                                       gstqmlgl
#                                        gstsoup
                                       gstudp
#                                       gstreamer-plugins-base-1.0 
#                                        gstreamer-video-1.0 
                                   )
    set (LIBS ${GST_PLUGINS_STATIC_LDFLAGS} 
              ${GSTGL_STATIC_LDFLAGS}
              EGL
              Qt5::AndroidExtras
              android
              log)
else (ANDROID)
    set(LIBS ${GSTREAMER_LDFLAGS})
endif()

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src src/*.cpp)

HEADER_DIRECTORIES(${CMAKE_SOURCE_DIR}/src DIRS)
include_directories(${DIRS} ${GSTREAMER_INCLUDE_DIRS})

file(GLOB_RECURSE QRC ${CMAKE_SOURCE_DIR}/src src/*.qrc)
qt5_add_resources(RESOURCES ${QRC})
set_property(SOURCE ${RESOURCES} PROPERTY SKIP_AUTOMOC ON)

if(ANDROID)
    include_directories(${ANDROID_SYSROOT}/usr/include)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${RESOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION 0.2)

else(ANDROID)
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/../common common)
target_link_libraries(${PROJECT_NAME} ${LIBS})
qt5_use_modules(${PROJECT_NAME} Core Qml Quick Svg Multimedia Network Bluetooth)

if(ANDROID)
    set (QT_ANDROID_APP_NAME ${PROJECT_NAME})
    include(3rdparty/qt-android-cmake/AddQtAndroidApk.cmake)
    
    add_qt_android_apk(${PROJECT_NAME}_apk
                        ${PROJECT_NAME}
                        NAME "Robotank"
                        PACKAGE_NAME "vitaliy.bondar.robotank"
                        PACKAGE_SOURCES ${CMAKE_SOURCE_DIR}/android
                        BUILDTOOLS_REVISION "25.0.0"
)
endif()
