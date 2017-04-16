import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.2

ToolButton {
    width: 32
    height: 32
    property alias imageSource: icon.source

    Image {
        id: icon
        anchors.fill: parent
        anchors.margins: 1
    }

    style: ButtonStyle {
        padding {
            left: 0
            right: 0
            top: 0
            bottom: 0
        }
    }
}
