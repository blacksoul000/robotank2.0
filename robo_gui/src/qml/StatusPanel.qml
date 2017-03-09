import QtQuick 2.0

Item {
    id: root
    height: 38

    property QtObject presenter

    Rectangle {
        color: "#616769"
        anchors.fill: parent

        Row {
            spacing: 20

            anchors.fill: parent
            anchors.margins: 2

            StatusIndicator {
                image: "qrc:/icons/azimuth.svg"
                imageWidth: 60
                text: presenter.gunPositionH
                anchors.verticalCenter: parent.verticalCenter
            }
            StatusIndicator {
                image: "qrc:/icons/angle.svg"
                text: presenter.gunPositionV
                anchors.verticalCenter: parent.verticalCenter
            }
            StatusIndicator {
                image: "qrc:/icons/pitch.svg"
                imageWidth: 60
                imageRotation: presenter.pitch
                text: presenter.pitch.toFixed(1)
                anchors.verticalCenter: parent.verticalCenter
            }
            StatusIndicator {
                image: "qrc:/icons/roll.svg"
                imageWidth: 35
                imageRotation: presenter.roll
                text: presenter.roll.toFixed(1)
                anchors.verticalCenter: parent.verticalCenter
            }

            BatteryIndicator {
                width: 45
                height: parent.height
                level: presenter.robotBatteryLevel
                image: "qrc:/icons/pitch.svg"
                anchors.verticalCenter: parent.verticalCenter
            }

            BatteryIndicator {
                width: 30
                height: parent.height
                level: presenter.gamepadBatteryLevel
                image: "qrc:/icons/gamepad.svg"
                anchors.verticalCenter: parent.verticalCenter
            }

            BatteryIndicator {
                width: 30
                height: parent.height
                level: presenter.batteryLevel
                charging: presenter.isCharging
                image: "qrc:/icons/phone.svg"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        DigitalClock {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}

