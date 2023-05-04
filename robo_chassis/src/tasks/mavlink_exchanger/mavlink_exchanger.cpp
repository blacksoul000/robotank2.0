#include "mavlink_exchanger.h"

#include "pub_sub.h"

#include "endpoint.h"
#include "udp_link.h"
#include "mavlink_communicator.h"
#include "communicator_builder.h"
#include "vehicle_registry.h"
#include "vehicle.h"

#include <mavlink.h>

#include <QNetworkConfigurationManager>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDebug>

class MavlinkExchanger::Impl
{
public:
    QNetworkConfigurationManager manager;
    domain::MavLinkCommunicator* communicator = nullptr;
    domain::VehiclePtr vehicle;

    Publisher< data_source::AbstractLinkPtr >* connectionP = nullptr;
};

MavlinkExchanger::MavlinkExchanger() :
    ITask(),
    d(new Impl)
{
    d->connectionP = PubSub::instance()->advertise< data_source::AbstractLinkPtr >("connection");
    PubSub::instance()->subscribe("vehicle", &MavlinkExchanger::onVehicleSelected, this);

    CommunicatorBuilder builder;
    builder.initCommunicator();
    builder.buildIdentification(42, 0, MAV_TYPE_TANK);
    builder.buildHandlers(); // later
    d->communicator = builder.getCommunicator();
    d->communicator->setParent(this);
}

MavlinkExchanger::~MavlinkExchanger()
{
    delete d->connectionP;
    delete d->communicator;
    delete d;
}

void MavlinkExchanger::start()
{
    connect(d->communicator->vehicleRegistry().data(), &domain::VehicleRegistry::vehicleAdded,
            this, &MavlinkExchanger::onVehicleAdded);
    connect(d->communicator->vehicleRegistry().data(), &domain::VehicleRegistry::vehicleRemoved,
            this, &MavlinkExchanger::onVehicleRemoved);
    connect(&d->manager, &QNetworkConfigurationManager::configurationChanged,
            this, &MavlinkExchanger::onNetworkConfigurationChanged);

    for (const QNetworkInterface& iface: QNetworkInterface::allInterfaces())
    {
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
            && !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            this->onInterfaceOnlineChanged(iface, !iface.addressEntries().empty());
        }
    }
}

void MavlinkExchanger::onVehicleAdded(domain::VehiclePtr vehicle)
{
    Q_UNUSED(vehicle)
}

void MavlinkExchanger::onVehicleRemoved(domain::VehiclePtr vehicle)
{
    if (!d->vehicle || !vehicle) return;
    if (vehicle->sysId() != vehicle->sysId()) return;

    disconnect(d->vehicle.data(), &domain::Vehicle::onlineChanged,
            this, &MavlinkExchanger::onVehicleOnlineChanged);
    d->vehicle.clear();
}

void MavlinkExchanger::onVehicleOnlineChanged()
{
    d->connectionP->publish((d->vehicle && d->vehicle->isOnline())
        ? d->vehicle->link() : data_source::AbstractLinkPtr());
}

void MavlinkExchanger::onVehicleSelected(const quint8& sysId)
{
    if (d->vehicle)
    {
        if (d->vehicle->sysId() != sysId)
        {
            disconnect(d->vehicle.data(), &domain::Vehicle::onlineChanged,
                this, &MavlinkExchanger::onVehicleOnlineChanged);
            d->vehicle.clear();
            this->onVehicleOnlineChanged();
        }
        else
        {
            return;
        }
    }

    if (sysId > 0)
    {
        d->vehicle = d->communicator->vehicleRegistry()->vehicle(sysId);
        if (d->vehicle)
        {
            connect(d->vehicle.data(), &domain::Vehicle::onlineChanged,
                    this, &MavlinkExchanger::onVehicleOnlineChanged);
            this->onVehicleOnlineChanged();
        }
    }
}

void MavlinkExchanger::onNetworkConfigurationChanged(const QNetworkConfiguration& config)
{
    auto iface = QNetworkInterface::interfaceFromName(config.name());
    if (!iface.isValid()) return;
    if (iface.flags().testFlag(QNetworkInterface::IsLoopBack)) return;

    this->onInterfaceOnlineChanged(iface, !iface.addressEntries().empty());
}

void MavlinkExchanger::onInterfaceOnlineChanged(const QNetworkInterface& iface, bool online)
{
    qDebug() << Q_FUNC_INFO << iface.name() << online;
    if (online)
    {
        for (const QNetworkAddressEntry& entry: iface.addressEntries())
        {
            if (!entry.broadcast().isNull())
            {
                qDebug() << "Found broadcast entry: " << entry.broadcast();
                auto link = new data_source::UdpLink(iface.name(), {entry.broadcast(), 14550},
                                                     {QHostAddress::AnyIPv4, 14550});
                d->communicator->addHeartbeatLink(link);
                link->connectLink();
            }
        }
    } else {
        for (const auto& link: d->communicator->heartbeatLinks())
        {
            if (link->interface() == iface.name())
            {
                qDebug() << "Try remove link: " << link;
                link->disconnectLink();
                d->communicator->removeHeartbeatLink(link);
            }
        }
    }
}