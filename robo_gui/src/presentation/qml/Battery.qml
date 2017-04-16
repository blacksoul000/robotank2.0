import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: root
    property int level: 0
    property bool charging: false
    property bool showText: true

    width: 30
    height: 14

    Item {
        id: border
        anchors.fill: parent

        property string color: (level > 10 || charging) ? "white" : "red"
        property int radius: 2

        Rectangle {
            id: borderRect
            anchors.fill: parent
            anchors.rightMargin: rect.width - rect.radius

            radius: border.radius
            border.color: border.color
            border.width: 2
            color: "transparent"

            Item {
                anchors.fill: parent
                anchors.margins: 3

                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    width: parent.width * level / 100

                    color: {
                        if (level > 50 || charging) {
                            "#13D671"
                        } else if (level > 20) {
                            "#E8E520"
                        } else "red"
                    }
                }
            }

            Text {
                text: level
                visible: !root.charging && root.showText
                font.pixelSize: parent.height / 2
                color: border.color
                anchors.centerIn: parent
            }
            Image {
                id: ico
                width: root.width
                height: width
                source: "qrc:/icons/charging.svg"
                visible: root.charging && level < 100
                anchors.centerIn: parent

                ColorOverlay {
                    anchors.fill: ico
                    source: ico
                    color: "white"
                }
            }
        }

        Rectangle {
            id: rect
            anchors.verticalCenter: borderRect.verticalCenter
            anchors.left: borderRect.right
            anchors.leftMargin: -radius

            radius: border.radius
            border.color: border.color
            border.width: 2
            width: borderRect.height / 2.5
            height: borderRect.height / 2
            color: "transparent"
        }
    }
}

