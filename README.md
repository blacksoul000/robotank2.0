# robotank

Rpi:
* Enable camera and i2c in raspi-config

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

