import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Window 2.0

Item {
    visible: true
    width: Screen.width
    height: Screen.height

    Rectangle {
        color: roboPalette.backgroundColor
        anchors.fill: parent
    }

    Item {
        id: roboPalette

        property color backgroundColor: "#212126"
        property color textColor: "white"
        property int captionTextSize: 32
        property int middleTextSize: 28
        property int textSize: 24
    }

    StackView {
        id: stackView
        x: 0
        y: 0
        width: parent.width
        height: parent.height
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
        onCurrentItemChanged: {
            if (currentItem)  currentItem.focus = true
        }

        delegate: StackViewDelegate {
//             function transitionFinished(properties){
//                 if(exitItem == initialItem) properties.exitItem.visible = true
//             }

            pushTransition: StackViewTransition {
                PropertyAnimation {
                    target: enterItem
                    property: "x"
                    from: -enterItem.width
                    to: 0
                    duration: 200
                }
            }
            popTransition: StackViewTransition {
	            PropertyAnimation {
	                target: exitItem
	                property: "x"
	                to: -exitItem.width
                    duration: 200
	            }
	        }
        }
    }
}
