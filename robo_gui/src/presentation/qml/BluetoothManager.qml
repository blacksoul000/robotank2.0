import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.2

import Robotank 1.0

Item {
    id: root
    
    property QtObject presenter: factory.bluetoothPresenter()
    
    Rectangle {
        id: rect
        color: roboPalette.backgroundColor
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 360
        
    //     title: qsTr("Bluetooth manager")
    
        ListView {
            id: view
            anchors.fill: parent
            model: presenter.devices
            delegate: itemDelegate
        }
    
        Component {
            id: itemDelegate
            Rectangle {
                color: roboPalette.backgroundColor
                border.color: roboPalette.textColor
                border.width: 1
    
                width: rect.width
                height: 60
    
                Column {
                    anchors.fill: parent
                    anchors.margins: 5
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
    
                Button {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 10
                    width: 100
                    style: ButtonStyle {
                        label: Text {
                            renderType: Text.NativeRendering
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: roboPalette.textSize
                            color: roboPalette.backgroundColor
                            text: modelData.isPaired ? qsTr("Disc.") : qsTr("Conn.")
                        }
                    }
                    onClicked: {
                        presenter.requestPair(modelData.address, !modelData.isPaired)
                    }
                }
            }
        }
    
        Button {
            anchors {
                margins: 5
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }
            
            style: ButtonStyle {
                label: Text {
                    renderType: Text.NativeRendering
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: roboPalette.textSize
                    color: roboPalette.backgroundColor
                    text: qsTr("Scan")
                }
            }
            
            BusyIndicator {
                anchors.fill: parent
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
