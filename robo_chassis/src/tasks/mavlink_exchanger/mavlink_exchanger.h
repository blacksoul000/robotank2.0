#ifndef MAVLINK_EXCHANGER_H
#define MAVLINK_EXCHANGER_H

#include "mavlink_traits.h"

#include "i_task.h"

class MavlinkExchanger : public ITask
{
public:
    explicit MavlinkExchanger();
    ~MavlinkExchanger();

    void start() override;

protected slots:
    void onVehicleAdded(domain::VehiclePtr vehicle);
    void onVehicleRemoved(domain::VehiclePtr vehicle);
    void onVehicleOnlineChanged(bool online);

private:
    class Impl;
    Impl* d;
};

#endif // MAVLINK_EXCHANGER_H
