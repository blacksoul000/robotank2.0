export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export ANDROID_NDK=/home/user/workspace/android/android-ndk-r10e
export ANDROID_SDK=/home/user/workspace/android/android-sdk-linux
export ANDROID_API_LEVEL=16
export CMAKE_PREFIX_PATH=/opt/Qt/5.9.1/android_armv7/lib/cmake

mkdir -p `pwd`/android-build
cd `pwd`/android-build
cmake .. -DCMAKE_TOOLCHAIN_FILE=`pwd`/../3rdparty/qt-android-cmake/toolchain/android.toolchain.cmake
make -j5

adb install -r ./bin/QtApp-debug.apk