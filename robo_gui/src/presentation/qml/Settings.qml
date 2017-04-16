import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import Robotank 1.0

Item {
    id: root
    width: grid.width + 5

    property int rowSpacing: 10
    property int columnSpacing: 30
    property QtObject presenter: factory.settingsPresenter()

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

    GridLayout {
        id: grid
        rowSpacing: root.rowSpacing
        columnSpacing: root.columnSpacing
        anchors.top: parent.top
        anchors.topMargin: 10
        flow: GridLayout.TopToBottom

        GroupBox {
            Layout.fillHeight: true
            Layout.fillWidth: true
            label: Text {
                color: roboPalette.textColor
                font.pixelSize: roboPalette.captionTextSize / 2
                text: qsTr("Video")
            }

            GridLayout {
                anchors.fill: parent
                rowSpacing: 10
                columnSpacing: 10
                columns: 2

                Text {
                    id: qualityLabel
                    color: roboPalette.textColor
                    font.pixelSize: roboPalette.textSize
                    text: "Quality"
                }
                RSpinBox {
                    height: qualityLabel.height
                    anchors.right: parent.right
                    inputValue: presenter.quality
                    onValueChanged: presenter.quality = value

                    Component.onCompleted: minimumValue = 1; // set value from presenter first
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
            label: Text {
                color: roboPalette.textColor
                font.pixelSize: roboPalette.captionTextSize / 2
                text: qsTr("Trackers")
            }

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
            label: Text {
                color: roboPalette.textColor
                font.pixelSize: roboPalette.captionTextSize / 2
                text: qsTr("Engine power") + ", %"
            }

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
                    Component.onCompleted: inputValue = presenter.enginePower(Engine.Left)
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
                    Component.onCompleted: inputValue = presenter.enginePower(Engine.Right)
                }

                Component.onCompleted: {
                    updateEnginePower()
                    presenter.enginePowerChanged.connect(updateEnginePower)
                }

                function updateEnginePower() {
                    leftEngine.value = presenter.enginePower(Engine.Left)
                    rightEngine.value = presenter.enginePower(Engine.Right)
                }
            }
        }

        GroupBox {
            Layout.fillHeight: true
            Layout.fillWidth: true
            label: Text {
                color: roboPalette.textColor
                font.pixelSize: roboPalette.captionTextSize / 2
                text: qsTr("Sensors calibration")
            }

            GridLayout {
                rowSpacing: root.rowSpacing
                columnSpacing: root.columnSpacing
                anchors.fill: parent

                Button {
                    text: "Gun"
                    onClicked: {
                        presenter.calibrateGun()
                    }
                }
                Button {
                    text: "Camera"
                    onClicked: {
                        presenter.calibrateCamera()
                    }
                }
                Button {
                    text: "Gyro"
                    onClicked: {
                        presenter.calibrateGyro()
                    }
                }
            }
        }
    }
}
