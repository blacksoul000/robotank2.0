#include "abstract_mavlink_handler.h"

//Internal
#include "mavlink_communicator.h"

using namespace domain;

AbstractMavLinkHandler::AbstractMavLinkHandler(MavLinkCommunicator* communicator):
    m_communicator(communicator)
{
    if (communicator) communicator->addHandler(this);
}

AbstractMavLinkHandler::~AbstractMavLinkHandler()
{
    if (m_communicator) m_communicator->removeHandler(this);
}
