import QtQuick 2.2
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.2

Item {
    id: page
    property string source;
    
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 360
        color: "green"//roboPalette.backgroundColor
    }        
}