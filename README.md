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

* Connect DualShock4 via bluetooth
  sudo apt-get install bluetooth bluez-utils blueman bluez python-gobject python-gobject-2
  run blueman-manager and connect device

* Install opencv-3 with contrib modules https://github.com/Itseez/opencv_contrib

* Install gstreamer1.0 with ugly plugins

Testing
  gst-launch-1.0 -v videotestsrc ! x264enc ! mpegtsmux ! udpsink host=localhost port=5000
  gst-launch-1.0 -v v4l2src ! x264enc ! mpegtsmux ! udpsink host=localhost port=5000
  gst-launch-1.0 -v v4l2src ! x264enc ! h264parse ! mpegtsmux ! udpsink host=localhost port=5000Testing

TODO
- add KCF tracker

