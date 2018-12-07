#ifndef SEND_ATTITUDE_HANDLER_H
#define SEND_ATTITUDE_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QScopedPointer>
#include <QPointF>
#include <QObject>

struct PointF3D;

namespace domain
{
    class SendAttitudeHandler: public QObject, public AbstractMavLinkHandler
    {
    public:
        SendAttitudeHandler(MavLinkCommunicator* communicator);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void onYpr(const PointF3D& ypr);
        void onGunPosition(const QPointF& position);

        void timerEvent(QTimerEvent *event) override;

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // SEND_ATTITUDE_HANDLER_H
