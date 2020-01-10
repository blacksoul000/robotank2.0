import QtQuick 2.0
import QtMultimedia 5.0
import QtQuick.Controls 1.4
import QtQuick.Window 2.2

Item {
    id: page
    
    property QtObject trackPresenter: factory.trackPresenter()
    property QtObject statusPresenter: factory.statusPresenter()
    property QtObject settingsPresenter: factory.settingsPresenter()
    property QtObject videoPresenter: factory.videoPresenter()
    property QtObject gamepadPresenter: factory.gamepadPresenter()
    property double scaleX: videoPresenter.width / page.width
    property double scaleY: videoPresenter.height / page.height

    property bool hasVideo: (videoPresenter.width > 0 && videoPresenter.height > 0)
    
    onVisibleChanged: visible = true // Always visible

    Item {
        id: videoContainer
        anchors.fill: parent;
        
        Component.onCompleted: {
            var item = videoPresenter.surface;
            item.parent = videoContainer;
            item.anchors.fill = videoContainer
        }
    }

    Crosshair {
        anchors.centerIn: parent
        visible: !trackPresenter.isTracking && page.hasVideo
        width: 150
        height: 150
    }

    CaptureArea {
        id: capture
        anchors.centerIn: parent
        visible: !trackPresenter.isTracking && page.hasVideo
        presenter: trackPresenter

        Connections {
            target: trackPresenter
            onCaptureSizeChanged: {
                trackPresenter.setCaptureFrameRect(screenToImage(capture.r))
            }
        }
    }

    Target {
        id: target
        x: trackPresenter.targetRect.x / page.scaleX
        y: trackPresenter.targetRect.y / page.scaleY
        width: trackPresenter.targetRect.width / page.scaleX
        height: trackPresenter.targetRect.height / page.scaleY
        visible: trackPresenter.isTracking
    }

    SelectArea {
        anchors.fill: parent
        visible: page.hasVideo
        onAccepted: {
            trackPresenter.onTrackRequest(screenToImage(r))
        }
    }

    StatusPanel {
        id: panel
        width: parent.width
        presenter: statusPresenter
        gamepad: gamepadPresenter
    }

    ChassisScheme {
        id: scheme
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 5
        yaw: statusPresenter.yaw
        azimuth: statusPresenter.gunPositionH
    }

    RToolButton {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 5
        width: 50
        height: 50

        imageSource: "qrc:/icons/settings.svg"
        onClicked: {
            stackView.push({
                item: Qt.resolvedUrl("qrc:/qml/Settings.qml"), 
                properties:{ presenter: settingsPresenter, 
                             gamepad: gamepadPresenter,
                             topMargin: panel.height + 5,
                             margins: 5
                            }})
        }
    }

    // convert screen coordinates to image
    function screenToImage(rect) {
        rect.x *= page.scaleX
        rect.y *= page.scaleY
        rect.width *= page.scaleX
        rect.height *= page.scaleY
        return rect
    }
}
