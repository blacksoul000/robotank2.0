#include "send_settings_handler.h"

#include "pub_sub.h"
#include "image_settings.h"

// Internal
#include "mavlink_communicator.h"
#include "abstract_link.h"
#include "vehicle_registry.h"
#include "vehicle.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QPoint>
#include <QDebug>

#include <string.h>

namespace
{
    const int gscMavId = 255; // FIXME

    /*
     * Copy src to string dst of size siz.  At most siz-1 characters
     * will be copied.  Always NUL terminates (unless siz == 0).
     * Returns strlen(src); if retval >= siz, truncation occurred.
     */
    size_t
    strlcpy(char *dst, const char *src, size_t siz)
    {
            char *d = dst;
            const char *s = src;
            size_t n = siz;

            /* Copy as many bytes as will fit */
            if (n != 0 && --n != 0) {
                    do {
                            if ((*d++ = *s++) == 0)
                                    break;
                    } while (--n != 0);
            }

            /* Not enough room in dst, add NUL and traverse rest of src */
            if (n == 0) {
                    if (siz != 0)
                            *d = '\0';                /* NUL-terminate dst */
                    while (*s++)
                            ;
            }

            return(s - src - 1);        /* count does not include NUL */
    }
}  // namespace

using namespace domain;
using data_source::AbstractLink;

struct SendSettingsHandler::Impl
{
    ImageSettings imageSettings;
    QPoint enginePower;
    quint8 selectedTracker;

    QString videoSource;
};

SendSettingsHandler::SendSettingsHandler(MavLinkCommunicator* communicator):
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    PubSub::instance()->subscribe("core/enginePower", &SendSettingsHandler::onEnginePowerChanged, this);
    PubSub::instance()->subscribe("camera/settings", &SendSettingsHandler::onImageSettingsChanged, this);
    PubSub::instance()->subscribe("tracker/selector", &SendSettingsHandler::onSwitchTrackerRequest, this);

    PubSub::instance()->subscribe("camera/source", &SendSettingsHandler::onVideoSourceChanged, this);
}

void SendSettingsHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_COMMAND_LONG) return;

    mavlink_command_long_t cmd;
    mavlink_msg_command_long_decode(&message, &cmd);

    if (cmd.command != MAV_CMD_REQUEST_SETTINGS) return;

    auto vehicle = m_communicator->vehicleRegistry()->vehicle(message.sysid);
    if (!vehicle || !vehicle->link()) return;

    mavlink_message_t reply;
    mavlink_settings_t settings;

    settings.image_quality = d->imageSettings.quality;
    settings.image_contrast = d->imageSettings.contrast;
    settings.image_brightness = d->imageSettings.brightness;
    settings.left_engine_power = d->enginePower.x();
    settings.right_engine_power = d->enginePower.y();
    settings.selected_tracker = d->selectedTracker;
    QByteArray ba = d->videoSource.toUtf8();
    ::strlcpy(settings.video_source, ba.data(), sizeof(settings.video_source));

    mavlink_msg_settings_encode(m_communicator->systemId(),
                                m_communicator->componentId(),
                                &reply, &settings);

    m_communicator->sendMessage(reply, vehicle->link());
//    qDebug() << Q_FUNC_INFO << "Send to " << link->send().address() << link->send().port() << message.msgid;
}

void SendSettingsHandler::onImageSettingsChanged(const ImageSettings& settings)
{
    d->imageSettings = settings;
}

void SendSettingsHandler::onEnginePowerChanged(const QPoint& enginePower)
{
    d->enginePower = enginePower;
}

void SendSettingsHandler::onSwitchTrackerRequest(const quint8& code)
{
    d->selectedTracker = code;
}

void SendSettingsHandler::onVideoSourceChanged(const QString& source)
{
    d->videoSource = source;
}
