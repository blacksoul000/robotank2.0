import QtQuick 2.0
import QtMultimedia 5.0
import QtQuick.Controls 1.4
//import QtSystemInfo 5.0

Item {
    id: page

    property QtObject trackPresenter: factory.trackPresenter()
    property QtObject statusPresenter: factory.statusPresenter()
    property double scaleX: video.sourceRect.width / page.width
    property double scaleY: video.sourceRect.height / page.height

    property bool hasVideo: (video.sourceRect.width > 0 && video.sourceRect.height > 0)

    onHasVideoChanged: console.log("has: ", hasVideo)

    MediaPlayer {
        id: player
        source: "rtsp://127.0.0.1:8554/live"
        autoPlay: true

        onErrorChanged: {
            if (error === MediaPlayer.NetworkError || error === MediaPlayer.ResourceError)
            {
                player.play();
            }
            else
            {
                console.log(errorString);
            }
        }
    }

    VideoOutput {
        id: video
        anchors.fill: parent;
        source: player;
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

    Loader {
        property int hidden: parent.x + parent.width
        property int showed: parent.x + parent.width - width
        property bool isLoaded: false
        property bool isHidden: true

        id: settingsLoader
        x: hidden
        anchors.top: panel.bottom
        anchors.bottom: parent.bottom

        PropertyAnimation {
            id: settingsShow
            target: settingsLoader
            property: "x"
            to: settingsLoader.showed
            duration: 200
        }
        PropertyAnimation {
            id: settingsHide
            target: settingsLoader
            property: "x"
            to: settingsLoader.hidden
            duration: 200
        }
    }

    StatusPanel {
        id: panel
        width: parent.width
        presenter: statusPresenter
    }

    ChassisScheme {
        id: scheme
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        yaw: statusPresenter.yaw
        azimuth: statusPresenter.gunPositionH
    }

    RButton {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 5
        width: 50
        height: 50

        imageSource: "qrc:/icons/settings.svg"
        onClicked: {
            if (!settingsLoader.isLoaded)
            {
                settingsLoader.isLoaded = true
                settingsLoader.source = "qrc:/qml/Settings.qml"
            }
            if (settingsLoader.isHidden)
            {
                settingsLoader.isHidden = false
                settingsShow.start()
            }
            else
            {
                settingsLoader.isHidden = true
                settingsHide.start()
            }
        }
    }
/*
    ScreenSaver {
        screenSaverEnabled: false;
    }
*/
    // convert screen coordinates to image
    function screenToImage(rect) {
        rect.x *= page.scaleX
        rect.y *= page.scaleY
        rect.width *= page.scaleX
        rect.height *= page.scaleY
        return rect
    }
}
