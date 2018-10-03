import QtQuick 2.2
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.2

import Robotank 1.0

Item {
    id: root
    
    property int rowSpacing: 10
    property int columnSpacing: 30
    property QtObject presenter
    property QtObject statusPresenter

    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 360
        color: roboPalette.backgroundColor
        
        ListModel {
            id: trackersModel
            ListElement {
                name: "Kcf"
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
            clip: true
    
            Column {
                id: col
                spacing: root.rowSpacing
    
                GroupBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    title: qsTr("Gamepad")
        //            label: Text {
        //                color: roboPalette.textColor
        //                font.pixelSize: roboPalette.captionTextSize / 2
        //                text: qsTr("Gamepad")
        //            }
    
                    Column {
                        anchors.fill: parent
                        spacing: root.rowSpacing
    
                        Row {
                            spacing: root.columnSpacing
    
                            Text {
                                color: roboPalette.textColor
                                font.pixelSize: roboPalette.textSize
                                text: qsTr("Status") + ":"
                            }
                            Text {
                                color: roboPalette.textColor
                                font.pixelSize: roboPalette.textSize
                                text: statusPresenter.gamepadStatus ? qsTr("Connected")
                                                                    : qsTr("Unconnected")
                            }
                        }
    
                        Button {
                            anchors.horizontalCenter: parent.horizontalCenter
                            style: ButtonStyle {
                                label: Text {
                                    renderType: Text.NativeRendering
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pixelSize: roboPalette.textSize
                                    color: roboPalette.backgroundColor
                                    text: qsTr("Manage")
                                }
                            }
                            onClicked: {
                                stackView.push({
                                    item: Qt.resolvedUrl("qrc:/qml/BluetoothManager.qml"), 
                                    properties:{}})
                            }
                        }
                    }
                }
    
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
                            id: brightnessLabel
                            color: roboPalette.textColor
                            font.pixelSize: roboPalette.textSize
                            text: "Brightness"
                        }
                        RSpinBox {
                            height: brightnessLabel.height
                            Layout.alignment: Qt.AlignRight
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
                            Layout.alignment: Qt.AlignRight
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
                            Layout.alignment: Qt.AlignRight
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
                            Layout.alignment: Qt.AlignRight
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
