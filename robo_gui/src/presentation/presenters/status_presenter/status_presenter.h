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
        Q_PROPERTY(int robotBatteryLevel READ robotBatteryLevel NOTIFY robotBatteryLevelChanged)
        Q_PROPERTY(qreal gunPositionH READ gunPositionH NOTIFY gunPositionHChanged)
        Q_PROPERTY(qreal gunPositionV READ gunPositionV NOTIFY gunPositionVChanged)
        Q_PROPERTY(qreal yaw READ yaw NOTIFY yawChanged)
        Q_PROPERTY(qreal pitch READ pitch NOTIFY pitchChanged)
        Q_PROPERTY(qreal roll READ roll NOTIFY rollChanged)
        Q_PROPERTY(bool arduinoStatus READ arduinoStatus NOTIFY arduinoStatusChanged)
        Q_PROPERTY(bool chassisStatus READ chassisStatus NOTIFY chassisStatusChanged)
        Q_PROPERTY(bool headlightStatus READ headlightStatus NOTIFY headlightStatusChanged)
        Q_PROPERTY(bool pointerStatus READ pointerStatus NOTIFY pointerStatusChanged)

        Q_PROPERTY(bool chassisImuOnline READ chassisImuOnline NOTIFY chassisImuStatusChanged)
        Q_PROPERTY(bool chassisImuReady READ chassisImuReady NOTIFY chassisImuStatusChanged)
        Q_PROPERTY(bool towerImuOnline READ towerImuOnline NOTIFY towerImuStatusChanged)
        Q_PROPERTY(bool towerImuReady READ towerImuReady NOTIFY towerImuStatusChanged)

        StatusPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~StatusPresenter() override;

        int batteryLevel() const;
        bool isCharging() const;
        int robotBatteryLevel() const;
        qreal gunPositionH() const;
        qreal gunPositionV() const;
        qreal yaw() const;
        qreal pitch() const;
        qreal roll() const;

        bool arduinoStatus() const;
        bool chassisStatus() const;

        bool chassisImuOnline() const;
        bool chassisImuReady() const;
        bool towerImuOnline() const;
        bool towerImuReady() const;

        bool headlightStatus() const;
        bool pointerStatus() const;

    signals:
        void batteryLevelChanged(int level);
        void isChargingChanged(bool charging);
        void robotBatteryLevelChanged(int level);
        void gunPositionHChanged(qreal position);
        void gunPositionVChanged(qreal position);
        void yawChanged(qreal yaw);
        void pitchChanged(qreal pitch);
        void rollChanged(qreal roll);

        void arduinoStatusChanged(bool online);
        void chassisStatusChanged(bool online);

        void headlightStatusChanged(bool on);
        void pointerStatusChanged(bool on);

        void chassisImuStatusChanged();
        void towerImuStatusChanged();

    private:
        class Impl;
        Impl* d;
    };
}

#endif //STATUS_PRESENTER_H
