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
```
sudo apt install libqt5serialport5-dev
cd robo_chassis
mkdir build
cd build
cmake .. && make mavlink && make -j4
```

Linux PC:
```
sudo apt install qtbase5-dev qtdeclarative5-dev libqt5svg5-dev \
    qtconnectivity5-dev libgstreamer1.0-dev libqt5gstreamer-dev \
    libgstreamer-plugins-good1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-qt5 qtmultimedia5-dev
pip3 install pymavlink
cd robo_gui
mkdir build
cd build
cmake .. && make mavlink && make -j4
```
