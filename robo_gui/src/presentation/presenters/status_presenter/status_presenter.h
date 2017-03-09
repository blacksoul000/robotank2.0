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
        Q_PROPERTY(int robotBatteryLevel READ robotBatteryLevel NOTIFY robotBatteryLevelChanged)
        Q_PROPERTY(int gunPositionH READ gunPositionH NOTIFY gunPositionHChanged)
        Q_PROPERTY(int gunPositionV READ gunPositionV NOTIFY gunPositionVChanged)
        Q_PROPERTY(int cameraPositionV READ cameraPositionV NOTIFY cameraPositionVChanged)
        Q_PROPERTY(qreal yaw READ yaw NOTIFY yawChanged)
        Q_PROPERTY(qreal pitch READ pitch NOTIFY pitchChanged)
        Q_PROPERTY(qreal roll READ roll NOTIFY rollChanged)

        StatusPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~StatusPresenter() override;

        int batteryLevel() const;
        bool isCharging() const;
        int gamepadBatteryLevel() const;
        int robotBatteryLevel() const;
        int gunPositionH() const;
        int gunPositionV() const;
        int cameraPositionV() const;
        qreal yaw() const;
        qreal pitch() const;
        qreal roll() const;

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

    private:
        class Impl;
        Impl* d;
    };
}

#endif //STATUS_PRESENTER_H
