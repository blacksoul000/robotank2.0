#ifndef SEND_GPS_RAW_HANDLER_H
#define SEND_GPS_RAW_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QObject>

namespace domain
{
    class SendGpsRawHandler: public QObject, public AbstractMavLinkHandler
    {
    public:
        SendGpsRawHandler(MavLinkCommunicator* communicator);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void timerEvent(QTimerEvent *event) override;

    private:
    };
}

#endif // SEND_GPS_RAW_HANDLER_H
