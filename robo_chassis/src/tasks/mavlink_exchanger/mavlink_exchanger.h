#ifndef MAVLINK_EXCHANGER_H
#define MAVLINK_EXCHANGER_H

#include "mavlink_traits.h"

#include "i_task.h"

class QNetworkConfiguration;
class QNetworkInterface;

class MavlinkExchanger : public ITask
{
public:
    explicit MavlinkExchanger();
    ~MavlinkExchanger();

    void start() override;

protected slots:
    void onVehicleAdded(domain::VehiclePtr vehicle);
    void onVehicleRemoved(domain::VehiclePtr vehicle);
    void onVehicleOnlineChanged();
    void onVehicleSelected(const quint8& sysId);
    void onNetworkConfigurationChanged(const QNetworkConfiguration& config);
    void onInterfaceOnlineChanged(const QNetworkInterface& iface, bool online);

private:
    class Impl;
    Impl* d;
};

#endif // MAVLINK_EXCHANGER_H
