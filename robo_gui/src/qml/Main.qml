import QtQuick 2.0
import QtQuick.Controls 1.4

Item {
    visible: true
    anchors.fill: parent

    Rectangle {
        color: roboPalette.backgroudColor
        anchors.fill: parent
    }

    Item {
        id: roboPalette

        property color backgroudColor: "#212126"
        property color textColor: "white"
        property int captionTextSize: 32
        property int textSize: 24
    }

    StackView {
        id: stackView
        anchors.fill: parent
        // Implements back key navigation
        focus: true
        initialItem: Qt.resolvedUrl("qrc:/qml/Combat.qml")
        Keys.onReleased: {
            if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
                if (stackView.depth > 1) {
                    stackView.pop();
                    event.accepted = true;
                } else {
                    Qt.quit();
                }
            }
        }
        delegate: StackViewDelegate {

            function transitionFinished(properties)
            {
                properties.exitItem.opacity = 1
            }

            pushTransition: StackViewTransition {
                PropertyAnimation {
                    target: enterItem
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 10
                }
                PropertyAnimation {
                    target: exitItem
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 10
                }
            }
        }
    }
}
