import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: root

    property int yaw: 0
    property int azimuth: 0

    height: 150
    width: 150

    Image {
        id: chassis
        width: 70
        height: 130
        source: "qrc:/icons/chassis.svg"
        anchors.centerIn: parent

        transform: Rotation {
            origin.x: chassis.width / 2;
            origin.y: chassis.height / 2;
            angle: root.yaw
        }
    }
    Image {
        id: tower
        width: 50
        height: 100
        source: "qrc:/icons/tower.svg"
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -30

        transform: Rotation {
            origin.x: tower.width / 2;
            origin.y: 4 * tower.height / 5;
            angle: root.azimuth
        }
    }
}

