export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export ANDROID_NDK=/home/user/workspace/android/android-ndk-r18b
export ANDROID_SDK=/home/user/workspace/android/android-sdk-linux
export ANDROID_HOME=/home/user/workspace/android/android-sdk-linux
export CMAKE_PREFIX_PATH=/opt/Qt5.12/5.12.0/android_armv7/lib/cmake
export SYSROOT=/home/user/workspace/android/toolchain/workspace/cerbero/build/dist/android_armv7
export PKG_CONFIG_PATH=${SYSROOT}/lib/pkgconfig:${SYSROOT}/lib/gstreamer-1.0/pkgconfig

mkdir -p `pwd`/android-build
cd `pwd`/android-build
cmake .. -DGSTREAMER_STATIC=ON \
        -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ON \
        -DANDROID_ABI=armeabi-v7a \
        -DANDROID_ARM_NEON=TRUE \
        -DANDROID_PLATFORM=android-21 \
        -DANDROID_STL_SHARED_LIBRARIES=c++_shared
make -j4 #&> 1.log

adb install -r ./build/outputs/apk/debug/android-build-debug.apk


# Honor9 libGLES_mali.so
# Sony X eglSubDriverAndroid.so libEGL_adreno.so libGLESv1_CM_adreno.so libGLESv2_adreno.so libQTapGLES.so libq3dtools_adreno.so libq3dtools_esx.so