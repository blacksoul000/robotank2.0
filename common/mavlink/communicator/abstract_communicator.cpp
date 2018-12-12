#include "abstract_communicator.h"

// Qt
#include <QDebug>

// Internal
#include "abstract_link.h"

using domain::AbstractCommunicator;
using data_source::AbstractLink;

AbstractCommunicator::AbstractCommunicator(QObject* parent):
    QObject(parent)
{}

AbstractCommunicator::~AbstractCommunicator()
{
    qDeleteAll(m_links);
}

QList<AbstractLink*> AbstractCommunicator::links() const
{
    return m_links;
}

void AbstractCommunicator::addLink(AbstractLink* link)
{
    link->setParent(this);
    m_links.append(link);

    connect(link, &AbstractLink::dataReceived, this, &AbstractCommunicator::onDataReceived);
    emit linkAdded(link);
}

void AbstractCommunicator::removeLink(AbstractLink* link)
{
    if (!m_links.removeOne(link)) return;

    disconnect(link, &AbstractLink::dataReceived, this, &AbstractCommunicator::onDataReceived);
    emit linkRemoved(link);
}
