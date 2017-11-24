import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: root
    height: 38

    property QtObject presenter

    Rectangle {
        color: "#616769"
        anchors.fill: parent

        Row {
            spacing: 10

            anchors.fill: parent
            anchors.margins: 2

            StatusIndicator {
                image: "qrc:/icons/azimuth.svg"
                imageWidth: 60
                text: presenter.gunPositionH.toFixed(1)
                anchors.verticalCenter: parent.verticalCenter
            }
            StatusIndicator {
                image: "qrc:/icons/angle.svg"
                text: presenter.gunPositionV.toFixed(1)
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
                image.source: "qrc:/icons/pitch.svg"
                anchors.verticalCenter: parent.verticalCenter

                ColorOverlay {
                    x: parent.image.x
                    y: parent.image.y
                    width: parent.image.width
                    height: parent.image.height
                    source: parent.image
                    color: presenter.chassisStatus ? "#00ce00" : "#ce0000"
                }
            }

            BatteryIndicator {
                width: 30
                height: parent.height
                level: presenter.gamepadBatteryLevel
                charging: presenter.gamepadCharging
                image.source: "qrc:/icons/gamepad.svg"
                anchors.verticalCenter: parent.verticalCenter

                ColorOverlay {
                    x: parent.image.x
                    y: parent.image.y
                    width: parent.image.width
                    height: parent.image.height
                    source: parent.image
                    color: presenter.gamepadStatus ? "#00ce00" : "#ce0000"
                }
            }

            BatteryIndicator {
                width: 30
                height: parent.height
                level: presenter.batteryLevel
                charging: presenter.isCharging
                image.source: "qrc:/icons/phone.svg"
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

