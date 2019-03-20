import QtQuick 2.0
import QtMultimedia 5.0
import QtQuick.Controls 1.4
import QtQuick.Window 2.2

//import QtSystemInfo 5.0

Item {
    id: page
    
    property QtObject trackPresenter: factory.trackPresenter()
    property QtObject statusPresenter: factory.statusPresenter()
    property QtObject settingsPresenter: factory.settingsPresenter()
    property QtObject videoPresenter: factory.videoPresenter()
    property double scaleX: 1
    property double scaleY: 1
    property bool hasVideo: true 
//     property double scaleX: video.sourceRect.width / page.width
//     property double scaleY: video.sourceRect.height / page.height

//     property bool hasVideo: (video.sourceRect.width > 0 && video.sourceRect.height > 0)
    
    onVisibleChanged: visible = true // Always visible

//     Timer {
//         id: restartTimer
//         interval: 1000; running: false; repeat: false
//         onTriggered: player.play();
//     }

//     MediaPlayer {
//         id: player
//         source: settingsPresenter.videoSource
// //        source: "udp://192.168.1.255:5000"
//         autoPlay: true
// 
//         onErrorChanged: {
//             if (error === MediaPlayer.NetworkError || error === MediaPlayer.ResourceError)
//             {
//                 restartTimer.running = true
//             }
//             else if (error !== MediaPlayer.NoError)
//             {
//                 console.log("Video error: ", errorString);
//             }
//         }
//         
//         onSourceChanged: console.log("Video source:", source)
//     }
//
//     VideoOutput {
//         id: video
//         anchors.fill: parent;
//         source: player;
//     }

    Item {
        id: videoContainer
        anchors.fill: parent;
        
        Component.onCompleted: {
            var item = videoPresenter.surface;
            item.parent = videoContainer;
            item.anchors.fill = videoContainer
//             
//             videoPresenter.play()
        }
        
        Timer {
            interval: 2000; running: true; repeat: false
            onTriggered: videoPresenter.play()
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
    }

    ChassisScheme {
        id: scheme
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 5
        yaw: statusPresenter.yaw
        azimuth: statusPresenter.gunPositionH
    }

    RButton {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 10
        width: 50
        height: 50

        imageSource: "qrc:/icons/settings.svg"
        onClicked: {
            stackView.push({
                item: Qt.resolvedUrl("qrc:/qml/Settings.qml"), 
                properties:{ presenter: settingsPresenter, 
                             statusPresenter: statusPresenter,
                             topMargin: panel.height + 10,
                             margins: 10
                            }})
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
