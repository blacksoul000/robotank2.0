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

using domain::MavlinkExchanger;

class MavlinkExchanger::Impl
{
public:
    domain::MavLinkCommunicator* communicator = nullptr;
    domain::RoboModel* model = nullptr;
};

MavlinkExchanger::MavlinkExchanger(domain::RoboModel* model, QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;
}

MavlinkExchanger::~MavlinkExchanger()
{
    delete d->communicator;
    delete d;
}

void MavlinkExchanger::start()
{
    CommunicatorBuilder builder(d->model);
    builder.initCommunicator();
    builder.buildIdentification(255, 0, MAV_TYPE_GCS);
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

void MavlinkExchanger::onVehicleAdded(VehiclePtr vehicle)
{
    qDebug() << Q_FUNC_INFO;
}

void MavlinkExchanger::onVehicleRemoved(VehiclePtr vehicle)
{
    qDebug() << Q_FUNC_INFO;
}
