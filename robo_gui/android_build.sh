export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export ANDROID_NDK=/home/user/workspace/android/android-ndk-r18b
export ANDROID_SDK=/home/user/workspace/android/android-sdk-linux
export ANDROID_HOME=/home/user/workspace/android/android-sdk-linux

mkdir -p `pwd`/android-build
cd `pwd`/android-build
cmake .. -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
        -DCMAKE_PREFIX_PATH=/opt/Qt5.12/5.12.0/android_armv7/lib/cmake \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ON \
        -DANDROID_ABI=armeabi-v7a \
        -DANDROID_ARM_NEON=TRUE \
        -DANDROID_PLATFORM=android-21 \
        -DANDROID_STL_SHARED_LIBRARIES=c++_shared
make -j4

adb install -r ./build/outputs/apk/debug/android-build-debug.apk