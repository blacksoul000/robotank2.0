export ANDROID_NDK=/home/blacksoul/workspace/android/android-ndk-r10e
export ANDROID_SDK=/home/blacksoul/workspace/android/android-sdk-linux
export ANDROID_API_LEVEL=15
export CMAKE_PREFIX_PATH=/opt/roscpp_output/target

mkdir -p `pwd`/android-build
cd `pwd`/android-build
cmake .. -DCMAKE_TOOLCHAIN_FILE=`pwd`/3rdparty/qt-android-cmake/toolchain/android.toolchain.cmake -DCATKIN_DEVEL_PREFIX=`pwd`/../../../devel
make -j5

adb install -r ./bin/QtApp-debug.apk