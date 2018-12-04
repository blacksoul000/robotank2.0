#include "mavlink_exchanger.h"

#include "endpoint.h"
#include "udp_link.h"
#include "mavlink_communicator.h"
#include "communicator_builder.h"
#include "vehicle_registry.h"
#include "vehicle.h"

#include <mavlink.h>

#include <QNetworkInterface>
#include <QHostAddress>
#include <QDebug>

class MavlinkExchanger::Impl
{
public:
    domain::MavLinkCommunicator* communicator = nullptr;
};

MavlinkExchanger::MavlinkExchanger() :
    ITask(),
    d(new Impl)
{}

MavlinkExchanger::~MavlinkExchanger()
{
    delete d->communicator;
    delete d;
}

void MavlinkExchanger::start()
{
    CommunicatorBuilder builder;
    builder.initCommunicator();
    builder.buildIdentification(42, 0, MAV_TYPE_GROUND_ROVER);
    builder.buildHandlers(); // later
    d->communicator = builder.getCommunicator();
    connect(d->communicator->vehicleRegistry().data(), &domain::VehicleRegistry::vehicleAdded,
            this, &MavlinkExchanger::onVehicleAdded);
    connect(d->communicator->vehicleRegistry().data(), &domain::VehicleRegistry::vehicleRemoved,
            this, &MavlinkExchanger::onVehicleRemoved);

    for (const QNetworkInterface& iface: QNetworkInterface::allInterfaces())
    {
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
                && !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            for (const QNetworkAddressEntry& entry: iface.addressEntries())
            {
                if (!entry.broadcast().isNull())
                {
                    auto link = new data_source::UdpLink({entry.broadcast(), 14550},
                                                         {QHostAddress::Any, 14550});
                    d->communicator->addHeartbeatLink(link);
                    link->connectLink();
                }
            }
        }
    }
}

void MavlinkExchanger::onVehicleAdded(domain::VehiclePtr vehicle)
{
    qDebug() << Q_FUNC_INFO;
}

void MavlinkExchanger::onVehicleRemoved(domain::VehiclePtr vehicle)
{
    qDebug() << Q_FUNC_INFO;
}
