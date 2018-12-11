#include "heartbeat_handler.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QMap>
#include <QTimerEvent>
#include <QBasicTimer>
#include <QDebug>

#include "mavlink_settings.h"

#include "mavlink_communicator.h"
#include "vehicle_registry.h"
#include "abstract_link.h"
#include "vehicle.h"
#include "endpoint.h"

using namespace domain;
using data_source::AbstractLink;
using data_source::Endpoint;

namespace
{
    Vehicle::Type decodeType(quint8 type)
    {
        switch (type) //TODO: other vehicles
        {
            case MAV_TYPE_FIXED_WING: return Vehicle::FixedWing;
            case MAV_TYPE_TRICOPTER: return Vehicle::Tricopter;
            case MAV_TYPE_QUADROTOR: return Vehicle::Quadcopter;
            case MAV_TYPE_HEXAROTOR: return Vehicle::Hexcopter;
            case MAV_TYPE_OCTOROTOR: return Vehicle::Octocopter;
            case MAV_TYPE_COAXIAL: return Vehicle::Coaxial;
            case MAV_TYPE_HELICOPTER: return Vehicle::Helicopter;
            case MAV_TYPE_VTOL_DUOROTOR:
            case MAV_TYPE_VTOL_QUADROTOR:
            case MAV_TYPE_VTOL_TILTROTOR:
            case MAV_TYPE_VTOL_RESERVED2:
            case MAV_TYPE_VTOL_RESERVED3:
            case MAV_TYPE_VTOL_RESERVED4:
            case MAV_TYPE_VTOL_RESERVED5: return Vehicle::Vtol;
            case MAV_TYPE_AIRSHIP:
            case MAV_TYPE_FREE_BALLOON: return Vehicle::Airship;
            case MAV_TYPE_KITE: return Vehicle::Kite;
            case MAV_TYPE_FLAPPING_WING: return Vehicle::Ornithopter;
            case MAV_TYPE_GROUND_ROVER: return Vehicle::Rover;
            case MAV_TYPE_GCS: return Vehicle::GCS;
            case MAV_TYPE_GENERIC:
            default: return Vehicle::UnknownType;
        }
    }

    const int heartbeatInterval = 1000; // ms
    const int systemOfflineTimeout = 3000; // ms
}  // namespace

class HeartbeatHandler::Impl
{
public:
    VehicleRegistryPtr registry;
    int sendTimer;

    QMap< int, QBasicTimer* > vehicleTimers;
};

HeartbeatHandler::HeartbeatHandler(MavLinkCommunicator* communicator):
    QObject(communicator),
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    d->registry = communicator->vehicleRegistry();
    d->sendTimer = this->startTimer(::heartbeatInterval);
}

HeartbeatHandler::~HeartbeatHandler()
{
    qDeleteAll(d->vehicleTimers);
}

void HeartbeatHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_HEARTBEAT) return;

    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&message, &heartbeat);

    domain::VehiclePtr vehicle = d->registry->vehicle(message.sysid);
    if (!vehicle)
    {
        qDebug() << Q_FUNC_INFO << "Creating vehicle" << message.sysid;
        vehicle = d->registry->addVehicle(message.sysid, nullptr);
    }

    if (!vehicle->isOnline())
    {
        const auto lastLink = m_communicator->lastReceivedLink();
        auto link = lastLink->clone(
                Endpoint(lastLink->send().address(), lastLink->send().port() + m_communicator->systemId()),
                Endpoint(QHostAddress::Any, lastLink->send().port() + message.sysid));

        link->connectLink();
        m_communicator->addLink(link);

        vehicle->setLink(link);
        vehicle->setType(::decodeType(heartbeat.type));
        vehicle->setOnline(true);

        auto timer = new QBasicTimer();
        d->vehicleTimers.insert(message.sysid, timer);

        qDebug() << Q_FUNC_INFO << "Added link" << vehicle->sysId()
                << link->receive().address() << link->receive().port()
                << link->send().address() << link->send().port();
        qDebug() << Q_FUNC_INFO << tr("Vehicle %1").arg(vehicle->name()) << tr("Online");
    }

    d->vehicleTimers.value(message.sysid)->start(::systemOfflineTimeout, this);
}

void HeartbeatHandler::sendHeartbeat()
{
    mavlink_message_t message;
    mavlink_heartbeat_t heartbeat;

    heartbeat.type = m_communicator->mavType();

    for (AbstractLink* link: m_communicator->heartbeatLinks())
    {
        mavlink_msg_heartbeat_encode_chan(m_communicator->systemId(),
                                          m_communicator->componentId(),
                                          m_communicator->linkChannel(link),
                                          &message, &heartbeat);
        m_communicator->sendMessage(message, link);
    }
}

void HeartbeatHandler::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == d->sendTimer)
    {
        this->sendHeartbeat();
    }
    else
    {
        for (QBasicTimer* timer: d->vehicleTimers.values())
        {
            if (timer->timerId() != event->timerId()) continue;

            auto vehicle = d->registry->vehicle(d->vehicleTimers.key(timer));
            if (!vehicle.isNull())
            {
                this->clearVehicleLink(vehicle);
                vehicle->setOnline(false);
                qDebug() << Q_FUNC_INFO << tr("Vehicle %1").arg(vehicle->name()) << tr("Offline");
            }
            timer->stop();
            delete timer;
            d->vehicleTimers.remove(d->vehicleTimers.key(timer));
        }
    }
}

void HeartbeatHandler::clearVehicleLink(const domain::VehiclePtr& vehicle)
{
    auto link = vehicle->link();
    link->disconnectLink();
    m_communicator->removeLink(link);
    vehicle->setLink(nullptr);
    delete link;
}
