#ifndef MAVLINK_EXCHANGER_H
#define MAVLINK_EXCHANGER_H

#include <QObject>

#include "mavlink_traits.h"

namespace domain
{
    class RoboModel;

    class MavlinkExchanger : public QObject
    {
    public:
        explicit MavlinkExchanger(RoboModel* model, QObject* parent = nullptr);
        ~MavlinkExchanger();

        void start();

    public slots:
        void onSelectedTrackerChanged(quint8 code);
        void onTrackToggle(const QRectF& rect);
        void onRequestScan();
        void onRequestPair(quint64 address, bool paired);
        void onEnginePowerChanged();
        void onCalibrateGun();
        void onCalibrateGyro();
        void onImageSettingsChanged();

    protected slots:
        void onVehicleAdded(VehiclePtr vehicle);
        void onVehicleRemoved(VehiclePtr vehicle);

        void onVehicleOnlineChanged(bool online);

    private:
        class Impl;
        Impl* d;
    };
}  // namespace domain

#endif // MAVLINK_EXCHANGER_H
