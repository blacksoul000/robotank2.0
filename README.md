# robotank

RPi:
* Enable camera and i2c in raspi-config

* Install opencv-3.4 with contrib modules https://github.com/Itseez/opencv_contrib

* Install gstreamer1.0 with ugly plugins
    
* Install pymavlink with pip
    pip3 install pymavlink    

* Add user to group bluetooth
    sudo usermod -a -G bluetooth pi

RPi:
cd robo_chassis
mkdir build
cd build
cmake .. && make mavlink && make -j4

Linux PC:
cd robo_gui
mkdir build
cd build
cmake .. && make mavlink && make -j4
