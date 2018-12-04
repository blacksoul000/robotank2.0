#ifndef COMMUNICATOR_BUILDER_H
#define COMMUNICATOR_BUILDER_H

#include "mavlink_communicator_builder.h"

namespace domain
{
    class RoboModel;
}  // namespace domain

class CommunicatorBuilder : public domain::MavLinkCommunicatorBuilder
{
public:
    explicit CommunicatorBuilder(domain::RoboModel* model);
    ~CommunicatorBuilder();
    void buildHandlers() override;

private:
    struct Impl;
    Impl* d;
};

#endif // COMMUNICATOR_BUILDER_H
