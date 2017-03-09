import QtQuick 2.0

Item {
    id: root
    width: 30
    height: 40

    property alias level: battery.level
    property alias charging: battery.charging
    property alias image: ico.source
    property alias imageHeight: ico.height

    Column {
        id: col
        anchors.fill: parent
        spacing: 2

        Image {
            id: ico
            width: parent.width
            height: 2 * (root.height - col.spacing) / 3
            source: "qrc:/icons/gamepad.svg"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Battery {
            id: battery
            height: (root.height - col.spacing) / 3
            showText: false
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
