import QtQuick 2.0

MouseArea {
    id: area
    anchors.fill: parent

    signal accepted(rect r)

    property int startPositionX
    property int startPositionY

    property rect r
    property int minSize: 10

    Rectangle {
        id: selection
        border.color: "black"
        color: "white"
        opacity: 0.3
        x: r.x
        y: r.y
        width: r.width
        height: r.height
        visible: (width > area.minSize && height > area.minSize)
    }

    onPressed: {
        r.x = mouseX
        r.y = mouseY
        r.width = 1
        r.height = 1
        startPositionX = mouseX
        startPositionY = mouseY
    }

    onReleased: {
        if (r.width < area.minSize || r.height < area.minSize) {
            reset()
        }
        accepted(r)
        reset()
    }

    onPositionChanged: {
        if (startPositionX > mouseX) {
            r.width = startPositionX - mouseX
            r.x = mouseX
        }
        else
        {
            r.width = mouseX - startPositionX
        }
        if (startPositionY > mouseY) {
            r.height = startPositionY - mouseY
            r.y = mouseY
        }
        else
        {
            r.height = mouseY - startPositionY
        }
    }

    function reset() {
        r.x = 0
        r.y = 0
        r.width = 0
        r.height = 0
    }
}
