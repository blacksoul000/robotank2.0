#include "mavlink_communicator.h"

// MAVLink
#include <mavlink.h>
#include <mavlink_types.h>
#include <mavlink_helpers.h>

// Qt
#include <QMap>
#include <QDebug>

// Internal
#include "abstract_link.h"
#include "abstract_mavlink_handler.h"
#include "endpoint.h"
#include "vehicle.h"
#include "vehicle_registry.h"
#include "command_service.h"

using namespace domain;
using data_source::AbstractLink;
using data_source::Endpoint;

class MavLinkCommunicator::Impl
{
public:
    data_source::AbstractLink* lastReceivedLink = nullptr;

    quint8 systemId = 255;
    quint8 componentId = 0;
    quint8 mavType = 0;

    QMap< data_source::AbstractLink*, quint8 > linkChannels;
    VehicleRegistryPtr vehicleRegistry;
    CommandServicePtr commandService;

    QList< quint8 > avalibleChannels;

    QList< AbstractMavLinkHandler* > handlers;
    QList< data_source::AbstractLink* > heartbeatLinks;
};

MavLinkCommunicator::MavLinkCommunicator(QObject* parent):
    AbstractCommunicator(parent),
    d(new Impl())
{
    qRegisterMetaType < mavlink_message_t > ("mavlink_message_t");
    d->vehicleRegistry = VehicleRegistryPtr::create();
    d->commandService = CommandServicePtr::create();

    for (quint8 channel = 0; channel < MAVLINK_COMM_NUM_BUFFERS; ++channel)
    {
        d->avalibleChannels.append(channel);
    }
}

MavLinkCommunicator::~MavLinkCommunicator()
{
    d->heartbeatLinks.clear();
    qDeleteAll(d->handlers);
    qDeleteAll(d->heartbeatLinks);
}

bool MavLinkCommunicator::isAddLinkEnabled()
{
    return !d->avalibleChannels.isEmpty();
}

quint8 MavLinkCommunicator::systemId() const
{
    return d->systemId;
}

quint8 MavLinkCommunicator::componentId() const
{
    return d->componentId;
}

quint8 MavLinkCommunicator::mavType() const
{
    return d->mavType;
}

quint8 MavLinkCommunicator::linkChannel(data_source::AbstractLink* link) const
{
    return d->linkChannels.value(link, 0);
}

data_source::AbstractLink* MavLinkCommunicator::lastReceivedLink() const
{
    return d->lastReceivedLink;
}

data_source::AbstractLink* MavLinkCommunicator::mavSystemLink(quint8 systemId)
{
    const auto& vehicle = d->vehicleRegistry->vehicle(systemId);
    if (vehicle) return vehicle->link();

    return nullptr;
}

VehicleRegistryPtr MavLinkCommunicator::vehicleRegistry() const
{
    return d->vehicleRegistry;
}

CommandServicePtr MavLinkCommunicator::commandService() const
{
    return d->commandService;
}

void MavLinkCommunicator::addHeartbeatLink(data_source::AbstractLink* link)
{
    if (d->heartbeatLinks.contains(link)) return;
    d->heartbeatLinks.append(link);

    link->setParent(this);

    connect(link, &AbstractLink::dataReceived, this, &MavLinkCommunicator::onDataReceived);
    emit linkAdded(link);
}

void MavLinkCommunicator::removeHeartbeatLink(data_source::AbstractLink* link)
{
    if (!d->heartbeatLinks.contains(link)) return;

    qDebug() << Q_FUNC_INFO << link->send().address();
    disconnect(link, &AbstractLink::dataReceived, this, &MavLinkCommunicator::onDataReceived);
    link->setParent(nullptr);

    d->heartbeatLinks.removeAll(link);
    emit linkRemoved(link);
}

void MavLinkCommunicator::addLink(data_source::AbstractLink* link)
{
    if (d->linkChannels.contains(link) || d->avalibleChannels.isEmpty()) return;

    d->linkChannels[link] = d->avalibleChannels.takeFirst();
    if (d->avalibleChannels.isEmpty()) emit addLinkEnabledChanged(false);

    AbstractCommunicator::addLink(link);
}

void MavLinkCommunicator::removeLink(data_source::AbstractLink* link)
{
    if (!d->linkChannels.contains(link)) return;
    const bool addLinkEnabled = this->isAddLinkEnabled();

    quint8 channel = d->linkChannels.value(link);
    d->linkChannels.remove(link);
    d->avalibleChannels.prepend(channel);

    d->vehicleRegistry->removeLink(link);
    AbstractCommunicator::removeLink(link);

    if (addLinkEnabled != this->isAddLinkEnabled())
    {
        emit addLinkEnabledChanged(this->isAddLinkEnabled());
    }
}

QList< data_source::AbstractLink* > MavLinkCommunicator::heartbeatLinks() const
{
    return d->heartbeatLinks;
}

void MavLinkCommunicator::setSystemId(quint8 systemId)
{
    if (d->systemId == systemId) return;

    d->systemId = systemId;
    emit systemIdChanged(systemId);
}

void MavLinkCommunicator::setComponentId(quint8 componentId)
{
    if (d->componentId == componentId) return;

    d->componentId = componentId;
    emit componentIdChanged(componentId);
}

void MavLinkCommunicator::setMavType(quint8 type)
{
    if (d->mavType == type) return;

    d->mavType = type;
    emit mavTypeChanged(type);
}

void MavLinkCommunicator::addHandler(AbstractMavLinkHandler* handler)
{
    d->handlers.append(handler);
}

void MavLinkCommunicator::removeHandler(AbstractMavLinkHandler* handler)
{
    d->handlers.removeOne(handler);
}

void MavLinkCommunicator::sendMessage(mavlink_message_t& message, AbstractLink* link)
{
    if (!link || !link->isConnected()) return;

    this->finalizeMessage(message);

    quint8 buffer[MAVLINK_MAX_PACKET_LEN];
    int lenght = mavlink_msg_to_send_buffer(buffer, &message);

    if (!lenght) return;
    link->sendData(QByteArray((const char*)buffer, lenght));
}

void MavLinkCommunicator::onDataReceived(const QByteArray& data)
{
    AbstractLink* link = qobject_cast< AbstractLink* >(this->sender());
    if (!link) return;

    mavlink_message_t message;
    mavlink_status_t status;

    quint8 channel = this->linkChannel(link);
    for (int pos = 0; pos < data.length(); ++pos)
    {
        if (!mavlink_parse_char(channel, (quint8)data[pos], &message, &status)) continue;
        if (message.sysid == d->systemId || message.sysid == 0) continue;

        d->lastReceivedLink = link;
//        mavlink_status_t* channelStatus = mavlink_get_channel_status(channel);
//        info->protocol = (channelStatus->flags & MAVLINK_STATUS_FLAG_IN_MAVLINK1)
//                             ? ProtocolVersion::MavLink1 : ProtocolVersion::MavLink2;

        for (AbstractMavLinkHandler* handler: d->handlers)
        {
            handler->processMessage(message);
        }
    }
}

void MavLinkCommunicator::finalizeMessage(mavlink_message_t& message)
{
    Q_UNUSED(message)
}
