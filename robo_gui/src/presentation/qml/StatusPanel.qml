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

			anchors {
//				left: parent.left
//				right: parent.right
//				verticalCenter: parent.verticalCenter
            	fill: parent
				margins: 2
            }

            StatusIndicator {
                image.source: "qrc:/icons/pitch.svg"
                image.width: 60
                imageRotation: presenter.pitch
                text: presenter.pitch.toFixed(1)
				anchors.verticalCenter: parent.verticalCenter
            }
            
            StatusIndicator {
            	image.source: "qrc:/icons/roll.svg"
				image.width: 35
                imageRotation: presenter.roll
                text: presenter.roll.toFixed(1)
				anchors.verticalCenter: parent.verticalCenter
            }

            StatusIndicator {
				image.source: "qrc:/icons/pitch.svg"
            	image.width: 60
                text: (presenter.robotBatteryLevel / 1000.0).toFixed(1) + "V"
				anchors.verticalCenter: parent.verticalCenter

                ColorOverlay {
                    source: parent.image
                    color: presenter.chassisStatus ? "#00ce00" : "#ce0000"
					x: parent.image.x
					y: parent.image.y
					width: parent.image.width
					height: parent.image.height
                }
            }

            StatusIndicator {
            	image.source: "qrc:/icons/gamepad.svg"
            	image.width: 35
                text: presenter.gamepadBatteryLevel + "%"
				anchors.verticalCenter: parent.verticalCenter

                ColorOverlay {
                    source: parent.image
                    color: presenter.gamepadStatus ? "#00ce00" : "#ce0000"
					x: parent.image.x
					y: parent.image.y
					width: parent.image.width
					height: parent.image.height
                }
            }
            
            Image {
            	source: "qrc:/icons/headlight.svg"
	            width: root.height
	            height: root.height
				anchors.verticalCenter: parent.verticalCenter
				
                ColorOverlay {
                    source: parent
                    color: presenter.headlightStatus ? "#00ce00" : "#000000"
					anchors.fill: parent
                }
            }
            
            Image {
            	source: "qrc:/icons/target.svg"
	            width: 30
	            height: 30
				anchors.verticalCenter: parent.verticalCenter
				
                ColorOverlay {
                    source: parent
                    color: presenter.pointerStatus ? "#00ce00" : "#000000"
					anchors.fill: parent
                }
            }
        }

        DigitalClock {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}

