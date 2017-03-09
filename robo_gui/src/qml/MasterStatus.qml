import QtQuick 2.0
import QtQuick.Controls 1.4

Rectangle {
    id: root
    property alias masterIp: txt.text

    signal tryConnect(string ip)
    signal exit()

    width: 200
    height: 100
    color: "#616769"

    Column {
        anchors.centerIn: parent

        spacing: 20
        TextField {
            id: txt
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Row {
            spacing: 10
            Button {
                text: "Connect"
                onClicked: {
                    root.tryConnect(txt.text)
                }
            }

            Button {
                text: "Exit"
                onClicked: {
                    root.exit()
                }
            }
        }
    }
}

