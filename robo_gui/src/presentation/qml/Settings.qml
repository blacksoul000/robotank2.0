import QtQuick 2.2
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.2

import Robotank 1.0

Rectangle {
    id: root
    anchors.top: parent.top
    anchors.right: parent.right
    width: flow.width + 5
    color: roboPalette.backgroundColor

    property int rowSpacing: 10
    property int columnSpacing: 30
    property QtObject presenter

    ListModel {
        id: trackersModel
        ListElement {
            name: "Camshift"
            code: 1
        }
        ListElement {
            name: "MedianFlow"
            code: 2
        }
        ListElement {
            name: "Boosting"
            code: 3
        }
        ListElement {
            name: "Mil"
            code: 4
        }
        ListElement {
            name: "Tld"
            code: 5
        }
        ListElement {
            name: "CustomTld"
            code: 6
        }
        ListElement {
            name: "OpenTld"
            code: 7
        }
    }

    Component {
        id: trackerDelegate
        Item {
            property int offset: 10;
            width: label.width + box.width + offset
            height: 20
            Row {
                id: itemRow
                anchors.fill: parent
                spacing: offset
                Rectangle {
                    id: box
                    width: 20
                    height: 20

                    anchors.verticalCenter: parent.verticalCenter

                    property bool checked: presenter.trackerCode === code

                    border.color: "white"
                    Image {
                        anchors.fill: parent
                        source: "qrc:/icons/ok.svg"
                        visible: box.checked
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (box.checked) return;
                            presenter.trackerCode = code
                        }
                    }
                }
                Text {
                    id: label
                    color: roboPalette.textColor
                    font.pixelSize: roboPalette.textSize
                    anchors.verticalCenter: parent.verticalCenter
                    text: name
                }
            }
        }
    }

    Flickable {
        id: flow
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: col.width
        contentHeight: col.height

        Column {
            id: col
            spacing: root.rowSpacing

            GroupBox {
                Layout.fillHeight: true
                Layout.fillWidth: true
                title: qsTr("Video")
    //            label: Text {
    //                color: roboPalette.textColor
    //                font.pixelSize: roboPalette.captionTextSize / 2
    //                text: qsTr("Video")
    //            }

                GridLayout {
                    anchors.fill: parent
                    rowSpacing: root.rowSpacing
                    columnSpacing: root.columnSpacing
                    columns: 2

                    Text {
                        id: videoSourceLabel
                        color: roboPalette.textColor
                        font.pixelSize: roboPalette.textSize
                        text: qsTr("Source")
                    }
                    TextInput {
                        height: videoSourceLabel.height
                        anchors.right: parent.right
                        color: roboPalette.textColor
                        font.pixelSize: roboPalette.textSize
                        text: presenter.videoSource
                        onEditingFinished: presenter.videoSource = text
                    }

                    Text {
                        id: brightnessLabel
                        color: roboPalette.textColor
                        font.pixelSize: roboPalette.textSize
                        text: "Brightness"
                    }
                    RSpinBox {
                        height: brightnessLabel.height
                        anchors.right: parent.right
                        inputValue: presenter.brightness
                        onValueChanged: presenter.brightness = value
                    }

                    Text {
                        id: contrastLabel
                        color: roboPalette.textColor
                        font.pixelSize: roboPalette.textSize
                        text: "Contrast"
                    }
                    RSpinBox {
                        height: contrastLabel.height
                        anchors.right: parent.right
                        inputValue: presenter.contrast
                        onValueChanged: presenter.contrast = value
                    }
                }
            }

            GroupBox {
                Layout.fillHeight: true
                Layout.fillWidth: true
                title: qsTr("Trackers")
    //            label: Text {
    //                color: roboPalette.textColor
    //                font.pixelSize: roboPalette.captionTextSize / 2
    //                text: qsTr("Trackers")
    //            }

                GridLayout {
                    rowSpacing: root.rowSpacing
                    columnSpacing: root.columnSpacing
                    anchors.fill: parent
                    columns: 2

                    Repeater {
                        anchors.fill: parent
                        model: trackersModel
                        delegate: trackerDelegate
                    }
                }
            }

            GroupBox {
                Layout.fillHeight: true
                Layout.fillWidth: true
                title: qsTr("Engine power") + ", %"
    //            label: Text {
    //                color: roboPalette.textColor
    //                font.pixelSize: roboPalette.captionTextSize / 2
    //                text: qsTr("Engine power") + ", %"
    //            }

                GridLayout {
                    rowSpacing: root.rowSpacing
                    columnSpacing: root.columnSpacing
                    anchors.fill: parent
                    columns: 2

                    Text {
                        id: leftEngineLabel
                        color: roboPalette.textColor
                        font.pixelSize: roboPalette.textSize
                        text: "Left"
                    }
                    RSpinBox {
                        id: leftEngine
                        height: leftEngineLabel.height
                        anchors.right: parent.right
                        onValueChanged: presenter.setEnginePower(Engine.Left, value)
                    }

                    Text {
                        id: rightEngineLabel
                        color: roboPalette.textColor
                        font.pixelSize: roboPalette.textSize
                        text: "Right"
                    }
                    RSpinBox {
                        id: rightEngine
                        height: rightEngineLabel.height
                        anchors.right: parent.right
                        onValueChanged: presenter.setEnginePower(Engine.Right, value)
                    }
                }
            }

            GroupBox {
                Layout.fillHeight: true
                Layout.fillWidth: true
                title: qsTr("Sensors calibration")
    //            label: Text {
    //                color: roboPalette.textColor
    //                font.pixelSize: roboPalette.captionTextSize / 2
    //                text: qsTr("Sensors calibration")
    //            }

                GridLayout {
                    rowSpacing: root.rowSpacing
                    columnSpacing: root.columnSpacing
                    anchors.fill: parent

                    Button {
                        style: ButtonStyle {
                            label: Text {
                                renderType: Text.NativeRendering
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: roboPalette.textSize
                                color: roboPalette.backgroundColor
                                text: qsTr("Gun")
                            }
                        }
                        onClicked: {
                            presenter.calibrateGun()
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
                                text: qsTr("Camera")
                            }
                        }
                        onClicked: {
                            presenter.calibrateCamera()
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
                                text: qsTr("Gyro")
                            }
                        }
                        onClicked: {
                            presenter.calibrateGyro()
                        }
                    }
                }
            }
        }
    }

    onPresenterChanged: {
        if (presenter) {
            updateEnginePower()
            presenter.enginePowerChanged.connect(updateEnginePower)
        }
    }

    function updateEnginePower() {
        leftEngine.inputValue = presenter.enginePower(Engine.Left)
        rightEngine.inputValue = presenter.enginePower(Engine.Right)
    }
}
