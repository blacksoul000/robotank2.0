#ifndef COMMUNICATOR_BUILDER_H
#define COMMUNICATOR_BUILDER_H

#include "mavlink_communicator_builder.h"

class CommunicatorBuilder : public domain::MavLinkCommunicatorBuilder
{
public:
    explicit CommunicatorBuilder();
    void buildHandlers() override;
};

#endif // COMMUNICATOR_BUILDER_H
