import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.2

import Robotank 1.0

Item {
    id: root
        
    property int topMargin: 0
    property int margins: 0
    
    property QtObject presenter
    
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
    
        ListView {
            id: view
            anchors.fill: parent
            anchors.margins: root.margins
            model: presenter.vehicles
            spacing: root.margins
            delegate: itemDelegate
        }
    
        Component {
            id: itemDelegate
            RButton {
                id: btn

                anchors.horizontalCenter: parent.horizontalCenter
                width: view.width
                
                highlighted: presenter.currentVehicle == modelData
                height: 60
    
                Column {
                    anchors.fill: parent
                    anchors.margins: root.margins
                    Text {
                        text: modelData.name
                        color: roboPalette.backgroundColor
                        font.pixelSize: roboPalette.textSize
                    }
                    Text {
                        text: modelData.type
                        color: roboPalette.backgroundColor
                        font.pixelSize: roboPalette.textSize / 2
                    }
                }

                onClicked: {
                    console.log(btn.highlighted, (btn.highlighted ? null : modelData))
                    presenter.currentVehicle = (btn.highlighted ? null : modelData)
                }
            }
        }
    }
}
