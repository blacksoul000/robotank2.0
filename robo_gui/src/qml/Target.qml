import QtQuick 2.0

Item {
    property bool valid: width > 0 && height > 0
    Rectangle {
        color: "transparent"
        border.color: "black"
        visible: parent.visible
        anchors.fill: parent
        anchors.bottomMargin: 1
        anchors.rightMargin: 1
    }
    Rectangle {
        color: "transparent"
        border.color: "white"
        visible: parent.visible
        anchors.fill: parent
        anchors.topMargin: 1
        anchors.leftMargin: 1
    }
}
