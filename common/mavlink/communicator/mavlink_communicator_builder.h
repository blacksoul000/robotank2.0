#ifndef MAVLINK_COMMUNICATOR_BUILDER_H
#define MAVLINK_COMMUNICATOR_BUILDER_H

// Qt
#include <QtGlobal>

//#include "service_traits.h"

namespace domain
{
    class MavLinkCommunicator;

    class MavLinkCommunicatorBuilder
    {
    public:
        MavLinkCommunicatorBuilder();
        virtual ~MavLinkCommunicatorBuilder();

        void initCommunicator();
        MavLinkCommunicator* getCommunicator();

        void buildIdentification(quint8 systemId, quint8 componentId, quint8 mavType);
        virtual void buildHandlers() = 0;

    protected:
        MavLinkCommunicator* m_communicator;
//        VehicleRegistryPtr m_registry;
    };
}

#endif // MAVLINK_COMMUNICATOR_BUILDER_H
