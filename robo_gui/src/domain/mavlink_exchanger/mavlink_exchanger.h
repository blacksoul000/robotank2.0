#ifndef MAVLINK_EXCHANGER_H
#define MAVLINK_EXCHANGER_H

#include <QObject>

#include "mavlink_traits.h"

class QNetworkConfiguration;
class QNetworkInterface;

namespace domain
{
    class RoboModel;

    class MavlinkExchanger : public QObject
    {
        Q_OBJECT
    public:
        explicit MavlinkExchanger(RoboModel* model);
        ~MavlinkExchanger();

        void start();
        QMap< int, VehiclePtr > vehicles() const;
        VehiclePtr currentVehicle() const;

    public slots:
        void setCurrentVehicle(const VehiclePtr& vehicle);
        void onSelectedTrackerChanged();
        void onTrackToggle(const QRectF& rect);
        void onRequestScan();
        void onRequestPair(quint64 address, bool paired);
        void onEnginePowerChanged();
        void onCalibrateGun();
        void onCalibrateGyro();
        void onImageSettingsChanged();
        void onJoyChanged();
        void onHandshake();
        void onDismiss();

    signals:
        void vehiclesChanged();
        void currentVehicleChanged();

    protected slots:
        void onVehicleAdded(VehiclePtr vehicle);
        void onVehicleRemoved(VehiclePtr vehicle);

        void onVehicleOnlineChanged();
        void onNetworkConfigurationChanged(const QNetworkConfiguration& config);
        void onInterfaceOnlineChanged(const QNetworkInterface& iface, bool online);

    private:
        class Impl;
        Impl* d;
    };
}  // namespace domain

#endif // MAVLINK_EXCHANGER_H
