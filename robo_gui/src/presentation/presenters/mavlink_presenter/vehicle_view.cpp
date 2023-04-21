#include "vehicle_view.h"

using namespace presentation;

VehicleView::VehicleView(const domain::VehiclePtr& vehicle)
{
    m_vehicle = vehicle;
    m_sysId = vehicle->sysId();
    m_name = vehicle->name();
    m_type = this->typeString(vehicle->type());;
    connect(vehicle.data(), &domain::Vehicle::sysIdChanged, this, [&](){
        m_sysId = vehicle ? vehicle->sysId() : 0;
        emit sysIdChanged();
    });

    connect(vehicle.data(), &domain::Vehicle::nameChanged, this, [&](){
        m_name = vehicle ? vehicle->name() : "None";
        emit nameChanged();
    });

    connect(vehicle.data(), &domain::Vehicle::onlineChanged, this, [&](){
        m_online = vehicle && vehicle->isOnline();
        emit onlineChanged();
    });

    connect(vehicle.data(), &domain::Vehicle::typeChanged, this, [&](){
        m_type = vehicle ? this->typeString(vehicle->type()) : "Unknown";
        emit typeChanged();
    });
}

int VehicleView::sysId() const
{
    return m_sysId;
}

QString VehicleView::name() const
{
    return m_name;
}

QString VehicleView::type() const
{
    return m_type;
}

QString VehicleView::typeString(domain::Vehicle::Type type) const
{
    switch (type)
    {
        case domain::Vehicle::GCS: return tr("Ground control station");

        case domain::Vehicle::FixedWing: return tr("Fixed wing");
        case domain::Vehicle::FlyingWing: return tr("Flying wing");

        case domain::Vehicle::Quadcopter: return tr("Quadcopter");
        case domain::Vehicle::Tricopter: return tr("Tricopter");
        case domain::Vehicle::Hexcopter: return tr("Hexcopter");
        case domain::Vehicle::Octocopter: return tr("Octocopter");

        case domain::Vehicle::Helicopter: return tr("Helicopter");
        case domain::Vehicle::Coaxial: return tr("Coaxial");

        case domain::Vehicle::Vtol: return tr("Vtol");

        case domain::Vehicle::Airship: return tr("Airship");
        case domain::Vehicle::Kite: return tr("Kite");
        case domain::Vehicle::Ornithopter: return tr("Ornithopter");

        case domain::Vehicle::Rover: return tr("Rover");
        case domain::Vehicle::Tank: return tr("Tank");
        
        default: break;
    }
    return tr("Unknown");
}

bool VehicleView::isOnline() const
{
    return m_online;
}

const domain::VehiclePtr& VehicleView::vehicle() const
{
    return m_vehicle;
}