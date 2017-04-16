import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: root

    property int yaw: 0
    property int azimuth: 0

    height: 80
    width: height

    Image {
        id: chassis
        width: 7 * root.width / 15
        height: 13 * root.height / 15
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
        width: root.width / 3
        height: 2 * root.height / 3
        source: "qrc:/icons/tower.svg"
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -root.height / 5

        transform: Rotation {
            origin.x: tower.width / 2;
            origin.y: 4 * tower.height / 5;
            angle: root.azimuth
        }
    }
}

