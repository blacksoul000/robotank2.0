import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

Row {
    id: root
    property int minimumValue: 0
    property int maximumValue: 100
    property int step: 1
    property int value: 0
    property int inputValue: 0

    property bool updateWhileHold: false

    spacing: 5

    RToolButton {
        id: down
        width: height
        height: parent.height
        imageSource: "qrc:/icons/minus.svg"
        enabled: root.inputValue > root.minimumValue
        onClicked: root.inputValue -= root.step
        onPressedChanged: onButtonPressed(down, pressed)
    }

    Text {
        height: parent.height
        width: 70
        color: roboPalette.textColor
        font.pixelSize: roboPalette.textSize
        text: "" + inputValue
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    RToolButton {
        id: up
        width: height
        height: parent.height
        imageSource: "qrc:/icons/plus.svg"
        enabled: root.inputValue < root.maximumValue
        onClicked: root.inputValue += root.step
        onPressedChanged: onButtonPressed(up, pressed)
    }

    Timer {
        id: hold
        interval: 500; running: false; repeat: false
        onTriggered: repeat.start()
    }

    Timer {
        id: repeat
        property var btn
        interval: 100; running: false; repeat: true
        onTriggered: {
            btn.clicked()
            if (root.updateWhileHold) root.value = root.inputValue
        }
    }

    function onButtonPressed(btn, pressed) {
        repeat.stop()

        if (pressed) {
            repeat.btn = btn
            hold.start()
        } else {
            hold.stop()
            root.value = root.inputValue
        }
    }
}
