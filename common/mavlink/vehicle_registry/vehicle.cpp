#include "vehicle.h"

using namespace domain;

int Vehicle::sysId() const
{
    return m_sysId;
}

void Vehicle::setSysId(int sysId)
{
    if (sysId == m_sysId) return;

    m_sysId = sysId;
    emit sysIdChanged();
}

QString Vehicle::name() const
{
    return m_name;
}

void Vehicle::setName(const QString& name)
{
    if (name == m_name) return;

    m_name = name;
    emit nameChanged();
}

data_source::AbstractLink* Vehicle::link() const
{
    return m_link;
}

void Vehicle::setLink(data_source::AbstractLink* link)
{
    if (link == m_link) return;

    m_link = link;
    emit linkChanged();
}

Vehicle::Type Vehicle::type() const
{
    return m_type;
}

void Vehicle::setType(Type type)
{
    if (type == m_type) return;

    m_type = type;
    emit typeChanged();
}

bool Vehicle::isOnline() const
{
    return m_online;
}

void Vehicle::setOnline(bool online)
{
    if (m_online == online) return;

    m_online = online;
    emit onlineChanged();
}
