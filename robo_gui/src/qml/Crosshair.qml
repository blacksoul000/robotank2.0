import QtQuick 2.0

Item {
    id: root
    property int margin: 5

    ListModel {
        id: model
        ListElement {
            colour: "white"
        }
        ListElement {
            colour: "black"
        }
    }

    Component {
        id: delegate
        Item {
            property int centerX: root.width / 2
            property int centerY: root.height / 2
            property int rayLengthX: centerX - root.margin
            property int rayLengthY: centerY - root.margin
            Rectangle {
                x: centerX + index
                y: centerY - root.margin - rayLengthY + index
                width: 1
                height: rayLengthY
                color: colour
            }
            Rectangle {
                x: centerX + root.margin + index
                y: centerY + index
                width: rayLengthX
                height: 1
                color: colour
            }
            Rectangle {
                x: centerX + index
                y: centerY + root.margin + index
                width: 1
                height: rayLengthY
                color: colour
            }
            Rectangle {
                x: centerX - root.margin - rayLengthX + index
                y: centerY + index
                width: rayLengthX
                height: 1
                color: colour
            }
        }
    }

    Repeater {
        model: model
        delegate: delegate
    }
}
