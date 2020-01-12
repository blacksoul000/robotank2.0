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
    emit vehicleChanged();
}

QString Vehicle::name() const
{
    return m_name;
}

void Vehicle::setName(const QString& name)
{
    if (name == m_name) return;

    m_name = name;
    emit vehicleChanged();
}

data_source::AbstractLink* Vehicle::link() const
{
    return m_link;
}

void Vehicle::setLink(data_source::AbstractLink* link)
{
    if (link == m_link) return;

    m_link = link;
    emit vehicleChanged();
}

Vehicle::Type Vehicle::type() const
{
    return m_type;
}

void Vehicle::setType(Type type)
{
    if (type == m_type) return;

    m_type = type;
    emit vehicleChanged();
}

QString Vehicle::typeString() const
{
    switch (m_type)
    {
        case GCS: return tr("Ground control station");

        case FixedWing: return tr("Fixed wing");
        case FlyingWing: return tr("Flying wing");

        case Quadcopter: return tr("Quadcopter");
        case Tricopter: return tr("Tricopter");
        case Hexcopter: return tr("Hexcopter");
        case Octocopter: return tr("Octocopter");

        case Helicopter: return tr("Helicopter");
        case Coaxial: return tr("Coaxial");

        case Vtol: return tr("Vtol");

        case Airship: return tr("Airship");
        case Kite: return tr("Kite");
        case Ornithopter: return tr("Ornithopter");

        case Rover: return tr("Rover");
        case Tank: return tr("Tank");
        
        default: break;
    }
    return tr("Unknown");
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
