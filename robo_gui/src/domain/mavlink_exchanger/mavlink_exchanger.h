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

    protected slots:
        void onVehicleAdded(VehiclePtr vehicle);
        void onVehicleRemoved(VehiclePtr vehicle);

    private:
        class Impl;
        Impl* d;
    };
}  // namespace domain

#endif // MAVLINK_EXCHANGER_H
