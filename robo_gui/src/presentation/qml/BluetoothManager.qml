import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.2

import Robotank 1.0

Dialog {
    visible: true
    title: qsTr("Bluetooth manager")

    property QtObject presenter: factory.bluetoothPresenter()

    contentItem: Rectangle {
        id: root
        color: roboPalette.backgroundColor
        implicitWidth: 500
        implicitHeight: 400

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

                width: root.width
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
                    width: 180
                    style: ButtonStyle {
                        label: Text {
                            renderType: Text.NativeRendering
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: roboPalette.textSize
                            color: roboPalette.backgroundColor
                            text: modelData.isPaired ? qsTr("Unconnect") : qsTr("Connect")
                        }
                    }
                    onClicked: {
                        presenter.requestPair(modelData.address, !modelData.isPaired)
                    }
                }
            }
        }

        Row {
            id: buttonBox
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: 5
            spacing: 10

            Button {
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
                onClicked: {
                    presenter.requestScan()
                }
            }
            Button {
                style: ButtonStyle {
                    label: Text {
                        renderType: Text.NativeRendering
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: roboPalette.textSize
                        color: roboPalette.backgroundColor
                        text: qsTr("Close")
                    }
                }
                onClicked: {
                    presenter.stop()
                    close()
                }
            }
        }

        ProgressBar {
            anchors.left: buttonBox.right
            anchors.right: parent.right
            anchors.verticalCenter: buttonBox.verticalCenter
            anchors.margins: 5
            indeterminate: true
            visible: presenter.scanStatus
        }
    }

    Component.onCompleted: presenter.start()
}
