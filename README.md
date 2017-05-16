# robotank

Rpi:
* Enable camera in raspi-config

* Disable serial console.
  sudo systemctl disable serial-getty@ttyAMA0.service
  In file /boot/cmdline.txt remove options referring to the serial port options referring to the serial port.
  So, this
    dwc_otg.lpm_enable=0 console=ttyAMA0,115200 kgdboc=ttyAMA0,115200 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline rootwait
  becomes this
    dwc_otg.lpm_enable=0 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline rootwait

  Reboot.

* Install opencv-3 with contrib modules https://github.com/Itseez/opencv_contrib

* Install gstreamer1.0 with ugly plugins

* Add apt source and install Qt
    deb https://twolife.be/raspbian/ jessie main qt

Testing:

  gst-launch-1.0 -v videotestsrc ! x264enc ! mpegtsmux ! udpsink host=localhost port=5000
  gst-launch-1.0 -v v4l2src ! x264enc ! mpegtsmux ! udpsink host=localhost port=5000
  gst-launch-1.0 -v v4l2src ! x264enc ! h264parse ! mpegtsmux ! udpsink host=localhost port=5000

  gst-launch-1.0 videotestsrc ! video/x-raw,width=1280,height=1024 ! videoconvert ! x264enc ! mpegtsmux ! udpsink host=localhost port=5000
  gst-launch-1.0 -v udpsrc port=5000 ! tsdemux ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! ximagesink

  gst-launch-1.0 videotestsrc ! videoconvert ! video/x-raw,width=800,height=600 ! jpegenc ! avimux ! udpsink host=localhost port=5000


  gst-launch-1.0 rtspsrc location=rtsp://127.0.0.1:8554/live ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! xvimagesink

  QT_DEBUG_PLUGINS=1 <application>

TODO
- add KCF tracker

