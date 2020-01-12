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
    property alias model: view.model
    
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
            model: trackersModel
            spacing: root.margins
            delegate: itemDelegate
        }
    
        Component {
            id: itemDelegate
            RButton {
                id: btn

                anchors.horizontalCenter: parent.horizontalCenter
                width: view.width
                
                highlighted: presenter.trackerCode == code
                height: 60
    
                Text {
                    text: name
                    anchors.fill: parent
                    anchors.margins: root.margins
                    color: roboPalette.backgroundColor
                    font.pixelSize: roboPalette.textSize
                }

                onClicked: {
                    presenter.trackerCode = code
                }
            }
        }
    }
}
