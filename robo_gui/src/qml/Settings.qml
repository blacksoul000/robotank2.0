import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4
import Robotank 1.0

Item {
    id: root

    Text {
        id: label
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Settings"
        color: roboPalette.textColor
        font.pixelSize: roboPalette.captionTextSize
    }

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

    ExclusiveGroup { id: selectedTracker }

    Component {
        id: trackerDelegate
        Item {
            width: parent.width / 2
            height: 20
            Row {
                anchors.fill: parent
                spacing: 10
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

    GroupBox {
        id: sliders
        title: "Video"
        width: parent.width

        anchors {
            top: label.bottom
            left: parent.left
            right: parent.right
            leftMargin: 5
            rightMargin: 5
            topMargin: 20
        }

        RowLayout {
            anchors.fill: parent

            spacing: 10

            Column {
                spacing: 10
                Text {
                    id: qualityLabel
                    color: roboPalette.textColor
                    font.pixelSize: roboPalette.textSize
                    text: "Quality"
                }
                Text {
                    id: brightnessLabel
                    color: roboPalette.textColor
                    font.pixelSize: roboPalette.textSize
                    text: "Brightness"
                }
                Text {
                    id: contrastLabel
                    color: roboPalette.textColor
                    font.pixelSize: roboPalette.textSize
                    text: "Contrast"
                }
            }
            Column {
                spacing: 10
                Layout.fillWidth: true
                RSpinBox {
                    width: parent.width
                    height: qualityLabel.height
                    inputValue: presenter.quality
                    onValueChanged: presenter.quality = value

                    Component.onCompleted: minimumValue = 1; // set value from presenter first
                }
                RSpinBox {
                    width: parent.width
                    height: brightnessLabel.height
                    inputValue: presenter.brightness
                    onValueChanged: presenter.brightness = value
                }
                RSpinBox {
                    width: parent.width
                    height: contrastLabel.height
                    inputValue: presenter.contrast
                    onValueChanged: presenter.contrast = value
                }
            }
        }
    }

    GroupBox {
        title: "Trackers"
        id: trackers
        width: parent.width

        anchors {
            top: sliders.bottom
            left: parent.left
            right: parent.right
            leftMargin: 5
            rightMargin: 5
            topMargin: 10
        }

        Grid {
            columns: 2
            spacing: 10
            width: parent.width
            height: trackerDelegate.height * trackersModel.count / columns

            Repeater {
                model: trackersModel
                delegate: trackerDelegate
            }
        }
    }

    GroupBox {
        id: engines
        title: "Engine power, %"
        width: parent.width

        anchors {
            top: trackers.bottom
            left: parent.left
            right: parent.right
            leftMargin: 5
            rightMargin: 5
            topMargin: 10
        }

        RowLayout {
            anchors.fill: parent

            spacing: 10

            Column {
                spacing: 10
                Text {
                    id: leftEngineLabel
                    color: roboPalette.textColor
                    font.pixelSize: roboPalette.textSize
                    text: "Left"
                }
                Text {
                    id: rightEngineLabel
                    color: roboPalette.textColor
                    font.pixelSize: roboPalette.textSize
                    text: "Right"
                }
            }
            Column {
                spacing: 10
                Layout.fillWidth: true
                RSpinBox {
                    id: leftEngine
                    width: parent.width
                    height: leftEngineLabel.height
                    onValueChanged: presenter.setEnginePower(Engine.Left, value)
                    Component.onCompleted: inputValue = presenter.enginePower(Engine.Left)
                }
                RSpinBox {
                    id: rightEngine
                    width: parent.width
                    height: rightEngineLabel.height
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
    }

    GroupBox {
        title: "Sensors calibration"
        width: parent.width

        anchors {
            top: engines.bottom
            left: parent.left
            right: parent.right
            leftMargin: 5
            rightMargin: 5
            topMargin: 10
        }

        Grid {
            anchors.fill: parent

            spacing: 10

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
