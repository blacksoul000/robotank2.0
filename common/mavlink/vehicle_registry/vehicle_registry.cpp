#include "vehicle_registry.h"

#include "abstract_link.h"
#include "vehicle.h"

// Qt
#include <QDebug>

using namespace domain;

class VehicleRegistry::Impl
{
public:
    QMap< int, VehiclePtr > vehicles;
};

VehicleRegistry::VehicleRegistry(QObject* parent):
    QObject(parent),
    d(new Impl())
{
    qRegisterMetaType< VehiclePtr >("VehiclePtr");
}

VehicleRegistry::~VehicleRegistry()
{}

VehiclePtr VehicleRegistry::vehicle(int systemId) const
{
    return d->vehicles.value(systemId, VehiclePtr());
}

bool VehicleRegistry::removeVehicle(int sysId)
{
    if (!d->vehicles.contains(sysId)) return false;

    auto vehicle = d->vehicles.take(sysId);
    vehicle->setOnline(false);
    emit vehicleRemoved(vehicle);
    return true;
}

bool VehicleRegistry::removeVehicle(const VehiclePtr& vehicle)
{
    if (!vehicle) return false;
    return this->removeVehicle(vehicle->sysId());
}

bool VehicleRegistry::removeLink(data_source::AbstractLink* link)
{
    if (!link) return false;

    for (auto it = d->vehicles.constBegin(), end = d->vehicles.constEnd(); it != end; ++it)
    {
        if (it.value()->link() == link)
        {
            it.value()->setLink(nullptr);
        }
    }

    return true;
}

VehiclePtr VehicleRegistry::addVehicle(int sysId, data_source::AbstractLink* link)
{
    if (d->vehicles.contains(sysId)) return VehiclePtr();

    VehiclePtr vehicle = VehiclePtr::create();

    vehicle->setSysId(sysId);
    vehicle->setName(tr("MAV %1").arg(sysId));
    vehicle->setType(Vehicle::Auto);
    vehicle->setLink(link);
    d->vehicles[sysId] = vehicle;
    emit vehicleAdded(vehicle);

    return vehicle;
}
