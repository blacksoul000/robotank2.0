import QtQuick 2.0

Text {
    id: clock
    color: "white"
    font.pixelSize: parent.height * 0.7

    property string format: "hh:mm:ss"

    Timer {
        interval: 1000; running: true; repeat: true;
        onTriggered: {
            clock.text = new Date().toLocaleTimeString(Qt.locale(), clock.format);
        }
        Component.onCompleted: {
            onTriggered()
        }
    }
}
