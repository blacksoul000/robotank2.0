import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.2

import Robotank 1.0

Item {
    id: root
        
    property int topMargin: 0
    property int margins: 0
    
    property QtObject presenter: factory.bluetoothPresenter()
    
    Rectangle {
        id: rect
        anchors {
            top: root.top
            bottom: root.bottom
            left: root.left
            topMargin: root.topMargin
            margins: root.margins
        }
        width: 380
        color: roboPalette.backgroundColor
        border.width: 1
        border.color: roboPalette.textColor
    //     title: qsTr("Bluetooth manager")
    
        ListView {
            id: view
            anchors.fill: parent
            anchors.margins: root.margins
            model: presenter.devices
            spacing: root.margins
            delegate: itemDelegate
        }
    
        Component {
            id: itemDelegate
            Rectangle {
                color: roboPalette.backgroundColor
                border.color: roboPalette.textColor
                border.width: 1
    
                anchors.horizontalCenter: parent.horizontalCenter
                width: rect.width - root.margins
                height: 60
    
                Column {
                    anchors.fill: parent
                    anchors.margins: root.margins
                    Text {
                        text: modelData.name
                        color: roboPalette.textColor
                        font.pixelSize: roboPalette.textSize
                    }
                    Text {
                        text: modelData.address
                        color: roboPalette.textColor
                        font.pixelSize: roboPalette.textSize / 2
                    }
                }
    
                RButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: root.margins
                    width: 100
                    text: modelData.isPaired ? qsTr("Disc.") : qsTr("Conn.")
                    onClicked: {
                        presenter.requestPair(modelData.address, !modelData.isPaired)
                    }
                }
            }
        }
    
        RButton {
            anchors {
                margins: root.margins
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }

            width: view.width
            text: qsTr("Scan")
            
            BusyIndicator {
                anchors.centerIn: parent
                running: presenter.scanStatus
            }
            
            onClicked: {
                presenter.requestScan()
            }
        }
    }

    Component.onCompleted: presenter.start()
    Component.onDestruction: presenter.stop()
}
