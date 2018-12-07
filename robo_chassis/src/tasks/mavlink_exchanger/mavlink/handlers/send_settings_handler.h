#ifndef SEND_SETTINGS_HANDLER_H
#define SEND_SETTINGS_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QScopedPointer>
#include <QObject>

struct Point;
struct ImageSettings;

namespace domain
{
    class SendSettingsHandler: public QObject, public AbstractMavLinkHandler
    {
    public:
        SendSettingsHandler(MavLinkCommunicator* communicator);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void onImageSettingsChanged(const ImageSettings& settings);
        void onEnginePowerChanged(const QPoint& enginePower);
        void onSwitchTrackerRequest(const quint8& code);
        void onVideoSourceChanged(const QString& source);

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // SEND_SETTINGS_HANDLER_H
