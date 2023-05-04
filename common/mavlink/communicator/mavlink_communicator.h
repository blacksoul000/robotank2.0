#ifndef MAVLINK_COMMUNICATOR_H
#define MAVLINK_COMMUNICATOR_H

#include "abstract_communicator.h"
#include "mavlink_traits.h"

// MAVLink
#include <mavlink_types.h>

namespace data_source
{
    class Endpoint;
}  // namespace data_source

namespace domain
{
    class AbstractMavLinkHandler;

    class MavLinkCommunicator: public AbstractCommunicator
    {
        Q_OBJECT

    public:
        enum ProtocolVersion
        {
            MavLink1,
            MavLink2
        };

        explicit MavLinkCommunicator(QObject* parent = nullptr);
        ~MavLinkCommunicator() override;

        bool isAddLinkEnabled() override;

        quint8 systemId() const;
        quint8 componentId() const;
        quint8 mavType() const;

        quint8 linkChannel(data_source::AbstractLink* link) const;

        data_source::Endpoint lastSender() const;
        data_source::AbstractLink* lastReceivedLink() const;
        data_source::AbstractLink* mavSystemLink(quint8 systemId);

        VehicleRegistryPtr vehicleRegistry() const;
        CommandServicePtr commandService() const;

        QList< data_source::AbstractLink* > heartbeatLinks() const;

        void addHeartbeatLink(data_source::AbstractLink* link);
        void removeHeartbeatLink(data_source::AbstractLink* link);

    public slots:
        void addLink(data_source::AbstractLink* link) override;
        void removeLink(data_source::AbstractLink* link) override;

        void setSystemId(quint8 systemId);
        void setComponentId(quint8 componentId);
        void setMavType(quint8 mavType);

        void addHandler(AbstractMavLinkHandler* handler);
        void removeHandler(AbstractMavLinkHandler* handler);

        void sendMessage(mavlink_message_t& message, data_source::AbstractLink* link);

    signals:
        void systemIdChanged(quint8 systemId);
        void componentIdChanged(quint8 componentId);
        void mavTypeChanged(quint8 mavType);

    protected slots:
        void onDataReceived(const QByteArray& data) override;

    protected:
        virtual void finalizeMessage(mavlink_message_t& message);

    private:
        class Impl;
        QScopedPointer<Impl> const d;

        Q_ENUM(ProtocolVersion)
    };
}

#endif // MAVLINK_COMMUNICATOR_H
