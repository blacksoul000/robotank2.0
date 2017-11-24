#ifndef STATUS_PRESENTER_H
#define STATUS_PRESENTER_H

#include <QObject>

namespace domain
{
    class RoboModel;
}

namespace presentation
{
    class StatusPresenter : public QObject
    {
        Q_OBJECT
    public:
        Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY batteryLevelChanged)
        Q_PROPERTY(bool isCharging READ isCharging NOTIFY isChargingChanged)
        Q_PROPERTY(int gamepadBatteryLevel READ gamepadBatteryLevel NOTIFY gamepadBatteryLevelChanged)
        Q_PROPERTY(bool gamepadCharging READ gamepadCharging NOTIFY gamepadChargingChanged)
        Q_PROPERTY(int robotBatteryLevel READ robotBatteryLevel NOTIFY robotBatteryLevelChanged)
        Q_PROPERTY(qreal gunPositionH READ gunPositionH NOTIFY gunPositionHChanged)
        Q_PROPERTY(qreal gunPositionV READ gunPositionV NOTIFY gunPositionVChanged)
        Q_PROPERTY(qreal cameraPositionV READ cameraPositionV NOTIFY cameraPositionVChanged)
        Q_PROPERTY(qreal yaw READ yaw NOTIFY yawChanged)
        Q_PROPERTY(qreal pitch READ pitch NOTIFY pitchChanged)
        Q_PROPERTY(qreal roll READ roll NOTIFY rollChanged)
        Q_PROPERTY(bool arduinoStatus READ arduinoStatus NOTIFY arduinoStatusChanged)
        Q_PROPERTY(bool gamepadStatus READ gamepadStatus NOTIFY gamepadStatusChanged)
        Q_PROPERTY(bool chassisStatus READ chassisStatus NOTIFY chassisStatusChanged)

        StatusPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~StatusPresenter() override;

        int batteryLevel() const;
        bool isCharging() const;
        int gamepadBatteryLevel() const;
        bool gamepadCharging() const;
        int robotBatteryLevel() const;
        qreal gunPositionH() const;
        qreal gunPositionV() const;
        qreal cameraPositionV() const;
        qreal yaw() const;
        qreal pitch() const;
        qreal roll() const;

        bool arduinoStatus() const;
        bool gamepadStatus() const;
        bool chassisStatus() const;

    signals:
        void batteryLevelChanged(int level);
        void isChargingChanged(bool charging);
        void gamepadBatteryLevelChanged(int level);
        void gamepadChargingChanged(bool charging);
        void robotBatteryLevelChanged(int level);
        void gunPositionHChanged(qreal position);
        void gunPositionVChanged(qreal position);
        void cameraPositionVChanged(qreal position);
        void yawChanged(qreal yaw);
        void pitchChanged(qreal pitch);
        void rollChanged(qreal roll);

        void arduinoStatusChanged(bool online);
        void gamepadStatusChanged(bool online);
        void chassisStatusChanged(bool online);

    private:
        class Impl;
        Impl* d;
    };
}

#endif //STATUS_PRESENTER_H
