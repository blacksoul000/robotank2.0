#ifndef STATUS_MODEL_H
#define STATUS_MODEL_H

#include <QObject>

namespace domain
{
    class StatusModel : public QObject
    {
        Q_OBJECT
    public:
        explicit StatusModel(QObject* parent = 0);
        ~StatusModel();

        void setBatteryLevel(int level);
        int batteryLevel() const;

        void setCharging(bool charging);
        bool isCharging() const;

        void setGamepadBatteryLevel(int level);
        int gamepadBatteryLevel() const;

        void setRobotBatteryLevel(int level);
        int robotBatteryLevel() const;

        void setGunPositionH(int position);
        int gunPositionH() const;

        void setGunPositionV(int position);
        int gunPositionV() const;

        void setCameraPositionV(int position);
        int cameraPositionV() const;

        void setYaw(qreal yaw);
        qreal yaw() const;

        void setPitch(qreal pitch);
        qreal pitch() const;

        void setRoll(qreal roll);
        qreal roll() const;

        void setArduinoStatus(bool status);
        bool arduinoStatus() const;

        void setJoyStatus(bool status);
        bool joyStatus() const;

    signals:
        void batteryLevelChanged(int level);
        void isChargingChanged(bool charging);
        void gamepadBatteryLevelChanged(int level);
        void robotBatteryLevelChanged(int level);
        void gunPositionHChanged(int position);
        void gunPositionVChanged(int position);
        void cameraPositionVChanged(int position);
        void yawChanged(qreal yaw);
        void pitchChanged(qreal pitch);
        void rollChanged(qreal roll);
        void arduinoStatusChanged(bool status);
        void joyStatusChanged(bool status);

    private:
        class Impl;
        Impl* d;
    };
} //domain

#endif // STATUS_MODEL_H
