#include "vehicle.h"

using namespace domain;

int Vehicle::sysId() const
{
    return m_sysId;
}

void Vehicle::setSysId(int sysId)
{
    m_sysId = sysId;
}

QString Vehicle::name() const
{
    return m_name;
}

void Vehicle::setName(const QString& name)
{
    m_name = name;
}

data_source::AbstractLink* Vehicle::link() const
{
    return m_link;
}

void Vehicle::setLink(data_source::AbstractLink* link)
{
    m_link = link;
}

Vehicle::Type Vehicle::type() const
{
    return m_type;
}

void Vehicle::setType(Type type)
{
    m_type = type;
}

bool Vehicle::isOnline() const
{
    return m_online;
}

void Vehicle::setOnline(bool online)
{
    if (m_online == online) return;

    m_online = online;
    emit onlineChanged(online);
}
