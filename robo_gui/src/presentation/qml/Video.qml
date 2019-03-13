import QtQuick 2.4

import org.freedesktop.gstreamer.GLVideoItem 1.0

Item {
    width: 640
    height: 480

    GstGLVideoItem {
        id: video
        objectName: "videoItem"
    }
}
