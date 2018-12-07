#include "send_status_handler.h"

#include "pub_sub.h"

// Internal
#include "mavlink_communicator.h"
#include "abstract_link.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QDebug>

namespace
{
    const int gscMavId = 255; // FIXME
}  // namespace

using namespace domain;
using data_source::AbstractLink;

struct SendStatusHandler::Impl
{
    quint8 status = 0;
    quint16 voltage = 0;
    quint8 gamepadCapacity = 0;
    quint16 buttons = 0;
};

SendStatusHandler::SendStatusHandler(MavLinkCommunicator* communicator):
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    this->startTimer(100); // 10 Hz

    PubSub::instance()->subscribe("arduino/status", &SendStatusHandler::onArduinoStatus, this);

    PubSub::instance()->subscribe("joy/buttons", &SendStatusHandler::onJoyButtons, this);
    PubSub::instance()->subscribe("joy/status", &SendStatusHandler::onJoyStatus, this);
    PubSub::instance()->subscribe("joy/capacity", &SendStatusHandler::onJoyCapacity, this);
    PubSub::instance()->subscribe("joy/charging", &SendStatusHandler::onJoyCharging, this);

    PubSub::instance()->subscribe("tracker/status", &SendStatusHandler::onTrackerStatus, this);

    PubSub::instance()->subscribe("chassis/voltage", &SendStatusHandler::onChassisVoltage, this);
    PubSub::instance()->subscribe("chassis/headlight", &SendStatusHandler::onHeadlightChanged, this);
    PubSub::instance()->subscribe("chassis/pointer", &SendStatusHandler::onPointerChanged, this);

    PubSub::instance()->subscribe("bluetooth/scanning", &SendStatusHandler::onBluetoothScanStatus, this);
}

void SendStatusHandler::processMessage(const mavlink_message_t& message)
{
    Q_UNUSED(message)
}

void SendStatusHandler::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)

    AbstractLink* link = m_communicator->mavSystemLink(::gscMavId);
    if (!link) return;

    mavlink_message_t message;
    mavlink_sys_status_t status;

    status.system_state = d->status;
    status.voltage = d->voltage;
    status.gamepad_capacity = d->gamepadCapacity;
    status.gamepad_buttons = d->buttons;

    mavlink_msg_sys_status_encode(m_communicator->systemId(),
                                  m_communicator->componentId(),
                                  &message, &status);

    m_communicator->sendMessage(message, link);
//    qDebug() << Q_FUNC_INFO << "Send to " << link->send().address() << link->send().port() << message.msgid;
}

void SendStatusHandler::onArduinoStatus(const bool& online)
{
    online ? d->status |= ARDUINO_ONLINE : d->status &= ~ARDUINO_ONLINE;
}

void SendStatusHandler::onJoyButtons(const quint16& buttons)
{
    d->buttons = buttons;
}

void SendStatusHandler::onJoyStatus(const bool& connected)
{
    connected ? d->status |= GAMEPAD_CONNECTED : d->status &= ~GAMEPAD_CONNECTED;
}

void SendStatusHandler::onJoyCapacity(const quint8& capacity)
{
    d->gamepadCapacity = capacity;
}

void SendStatusHandler::onJoyCharging(const bool& charging)
{
    charging ? d->status |= GAMEPAD_CHARGING : d->status &= ~GAMEPAD_CHARGING;
}

void SendStatusHandler::onTrackerStatus(const bool& tracking)
{
    tracking ? d->status |= TRACKING : d->status &= ~TRACKING;
}

void SendStatusHandler::onChassisVoltage(const quint16& voltage)
{
    d->voltage = voltage;
}

void SendStatusHandler::onHeadlightChanged(const bool& on)
{
    on ? d->status |= HEADLIGHT : d->status &= ~HEADLIGHT;
}

void SendStatusHandler::onPointerChanged(const bool& on)
{
    on ? d->status |= BLUETOOTH_SCANNING : d->status &= ~BLUETOOTH_SCANNING;
}

void SendStatusHandler::onBluetoothScanStatus(const bool& scanning)
{
    scanning ? d->status |= POINTER : d->status &= ~POINTER;
}