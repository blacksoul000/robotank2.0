import QtQuick 2.0

Item {
    id: root
    height: 18
    width: row.childrenRect.width

    property alias image: ico
    property alias text: label.text
    property real imageRotation: 0

    Row {
        id: row
        anchors.fill: parent
        spacing: 5
        Image {
            id: ico
            width: 30
            height: 30
            anchors.verticalCenter: parent.verticalCenter

            transform: Rotation {
                origin.x: ico.width / 2;
                origin.y: ico.height / 5;
                angle: root.imageRotation
            }
        }
        Text {
            id: label
            width: 30
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 10
            color: "white"
        }
    }
}

