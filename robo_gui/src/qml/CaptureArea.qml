import QtQuick 2.0

Item {
    id: root
    property QtObject presenter
    property rect r
    property int rayLength: 14
    width: presenter.captureSize.width
    height: presenter.captureSize.height

    onXChanged: r.x = x
    onYChanged: r.y = y
    onWidthChanged: r.width = width
    onHeightChanged: r.height = height

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
            //top left
            Rectangle {
                x: index
                y: index
                width: 1
                height: rayLength
                color: colour
            }
            Rectangle {
                x: index
                y: index
                width: rayLength
                height: 1
                color: colour
            }
            //top right
            Rectangle {
                x: root.width - index
                y: index
                width: 1
                height: rayLength
                color: colour
            }
            Rectangle {
                x: root.width - rayLength - index
                y: index
                width: rayLength
                height: 1
                color: colour
            }
            //bottom right
            Rectangle {
                x: root.width - index
                y: root.height - rayLength - index
                width: 1
                height: rayLength
                color: colour
            }
            Rectangle {
                x: root.width - rayLength - index + 1
                y: root.height - index
                width: rayLength
                height: 1
                color: colour
            }
            //bottom left
            Rectangle {
                x: index
                y: root.height - rayLength - index
                width: 1
                height: rayLength
                color: colour
            }
            Rectangle {
                x: index
                y: root.height - index
                width: rayLength
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
