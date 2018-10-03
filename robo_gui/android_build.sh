export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export CMAKE_PREFIX_PATH=/opt/Qt/5.11.1/android_armv7/lib/cmake
export ANDROID_NDK=/home/user/workspace/android/android-ndk-r10e
#export ANDROID_NDK=/home/user/workspace/android/android-ndk-r17b
export ANDROID_SDK=/home/user/workspace/android/android-sdk-linux
export ANDROID_HOME=/home/user/workspace/android/android-sdk-linux
export ANDROID_API_LEVEL=21
export ANDROID_ARM_NEON=TRUE
export ANDROID_ABI="armeabi-v7a with NEON"

target_host=arm-linux-androideabi
export CMAKE_C_COMPILER=$target_host-gcc
export CMAKE_CXX_COMPILER=$target_host-g++

mkdir -p `pwd`/android-build
cd `pwd`/android-build
cmake .. -DCMAKE_TOOLCHAIN_FILE=`pwd`/../3rdparty/qt-android-cmake/toolchain/android.toolchain.cmake \
            -DCMAKE_MAKE_PROGRAM=make -DCMAKE_C_STANDARD_COMPUTED_DEFAULT=GNU -DCMAKE_CXX_STANDARD_COMPUTED_DEFAULT=GNU \
            -DANDROID_ABI=armeabi-v7a
#            -DANDROID_ABI="armeabi-v7a with NEON"
make -j5

#adb install -r ./bin/QtApp-debug.apk