#include "mavlink_exchanger.h"

#include "robo_model.h"
#include "status_model.h"

#include "endpoint.h"
#include "udp_link.h"
#include "mavlink_communicator.h"
#include "communicator_builder.h"
#include "vehicle_registry.h"
#include "vehicle.h"
#include "command_service.h"
#include "command.h"
#include "mavlink_traits.h"

#include <mavlink.h>

#include <QNetworkInterface>
#include <QHostAddress>
#include <QRectF>
#include <QDebug>

using domain::MavlinkExchanger;
using domain::Command;

class MavlinkExchanger::Impl
{
public:
    domain::MavLinkCommunicator* communicator = nullptr;
    domain::RoboModel* model = nullptr;

    int vehicleSysId = 0;
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

#include <QTimer>  //FIXME
void MavlinkExchanger::onVehicleAdded(VehiclePtr vehicle)
{
    if (d->vehicleSysId != 0) return;
    d->vehicleSysId = vehicle->sysId();

    // FIXME - onVehicle online
    d->model->status()->setChassisStatus(true);

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAV_CMD_REQUEST_SETTINGS);
    d->communicator->commandService()->executeCommand(d->vehicleSysId, command);

    //FIXME
    QTimer::singleShot(5000, [&](){
        qDebug() << Q_FUNC_INFO;
        this->onTrackToggle(QRectF(12, 34, 56, 78));
    });
}

void MavlinkExchanger::onVehicleRemoved(VehiclePtr vehicle)
{
    if (d->vehicleSysId != vehicle->sysId()) return;

    d->vehicleSysId = 0;

    // FIXME - onVehicle offline
//    d->model->status()->setChassisStatus(false);
}

void MavlinkExchanger::onSelectedTrackerChanged(quint8 code)
{
    if (d->vehicleSysId == 0) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAV_CMD_SET_TRACKER_CODE);
    command->addArgument(code);

    d->communicator->commandService()->executeCommand(d->vehicleSysId, command);
}

void MavlinkExchanger::onTrackToggle(const QRectF& rect)
{
    if (d->vehicleSysId == 0) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAV_CMD_TRACK);
    command->addArgument(rect.x());
    command->addArgument(rect.y());
    command->addArgument(rect.width());
    command->addArgument(rect.height());

    d->communicator->commandService()->executeCommand(d->vehicleSysId, command);
}
