import QtQuick 2.2
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Material 2.12

import Robotank 1.0

Item {
    id: root
    
    property int topMargin: 0
    property int margins: 0
    
    property int rowSpacing: 10
    property int columnSpacing: 30
    property QtObject presenter
    property QtObject gamepad
    property QtObject mavlink: factory.mavlinkPresenter()

    Material.theme: Material.Dark
    Material.accent: Material.Purple
    
    Rectangle {
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
    
        Flickable {
            id: flow
            anchors.fill: parent
            anchors.margins: root.margins
            contentHeight: col.height
            clip: true
    
            Column {
                id: col
                spacing: root.rowSpacing
                anchors.fill: parent

                RButton {
                    height: 50
                    width: parent.width

                    property QtObject vehicle: mavlink.currentVehicle

                    text: qsTr("Vehicle") + ": " +
                         (vehicle ? vehicle.name + " (" + vehicle.typeString + ")" : qsTr("None"))

                    onClicked: {
                        stackView.push({
                            item: Qt.resolvedUrl("qrc:/qml/VehicleManager.qml"), 
                            properties:{
                                topMargin: root.topMargin, 
                                margins: root.margins,
                                presenter: root.mavlink
                            }})
                    }
                }

                RButton {
                    height: 50
                    width: parent.width

                    property QtObject vehicle: mavlink.currentVehicle

                    text: qsTr("Gamepad") + ": " +
                         (gamepad.connected ? qsTr("Connected"): qsTr("Unconnected"))

                    onClicked: {
                        stackView.push({
                            item: Qt.resolvedUrl("qrc:/qml/BluetoothManager.qml"), 
                            properties:{topMargin: root.topMargin, 
                                        margins: root.margins 
                            }})
                    }
                }

                RButton {
                    id: trackerBtn
                    height: 50
                    width: parent.width
                    property var tracker
                    property int code: presenter.trackerCode

                    text: qsTr("Tracker") + ": " + (tracker ? tracker.name : qsTr("None"))

                    onCodeChanged: {
                        for (var i = 0; i < trackersModel.count; ++i)
                        {
                            var item = trackersModel.get(i)
                            if (trackerBtn.code == item.code)
                            {
                                trackerBtn.tracker = item
                                break
                            }
                        }
                    }

                    onClicked: {
                        stackView.push({
                            item: Qt.resolvedUrl("qrc:/qml/TrackerManager.qml"), 
                            properties:{topMargin: root.topMargin, 
                                        margins: root.margins,
                                        presenter: presenter,
                                        model: trackersModel
                            }})
                    }
                }
    
                GroupBox {
                    Layout.fillHeight: true
                    width: parent.width
                    title: qsTr("Video")
    
                    GridLayout {
                        anchors.fill: parent
                        rowSpacing: root.rowSpacing
                        columnSpacing: root.columnSpacing
                        columns: 2
    
                        Text {
                            id: brightnessLabel
                            color: roboPalette.textColor
                            font.pixelSize: roboPalette.textSize
                            text: qsTr("Brightness")
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
                            text: qsTr("Contrast")
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
                    width: parent.width
                    title: qsTr("Engine power") + ", %"
    
                    GridLayout {
                        rowSpacing: root.rowSpacing
                        columnSpacing: root.columnSpacing
                        anchors.fill: parent
                        columns: 2
    
                        Text {
                            id: leftEngineLabel
                            color: roboPalette.textColor
                            font.pixelSize: roboPalette.textSize
                            text: qsTr("Left")
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
                            text: qsTr("Right")
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
                    width: parent.width
                    title: qsTr("Sensors calibration")
    
                    RowLayout {
                        spacing: root.rowSpacing
                        anchors.fill: parent

                        RButton {
                            Layout.fillWidth: true
                            text: qsTr("Gun")
                            onClicked: {
                                presenter.calibrateGun()
                            }
                        }
                        RButton {
                            Layout.fillWidth: true
                            text: qsTr("Gyro")
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
