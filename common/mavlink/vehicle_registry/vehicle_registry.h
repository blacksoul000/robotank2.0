#ifndef VEHICLE_REGISTRY_H
#define VEHICLE_REGISTRY_H

// Qt
#include <QObject>
#include <QSharedPointer>

// Internal
#include "mavlink_traits.h"

namespace data_source
{
    class AbstractLink;
}  // namespace data_source

namespace domain
{
    class VehicleRegistry: public QObject
    {
        Q_OBJECT

    public:
        explicit VehicleRegistry(QObject* parent = nullptr);
        ~VehicleRegistry() override;

        VehiclePtr vehicle(int systemId) const;
        bool removeLink(data_source::AbstractLink* link);

        QMap< int, VehiclePtr > vehicles() const;

    public slots:
        bool removeVehicle(int sysId);
        bool removeVehicle(const VehiclePtr& vehicle);
        VehiclePtr addVehicle(int sysId, data_source::AbstractLink* link);

    signals:
        void vehicleAdded(VehiclePtr vehicle);
        void vehicleRemoved(VehiclePtr vehicle);

    private:
        class Impl;
        QScopedPointer<Impl> const d;
    };
}

#endif // VEHICLE_REGISTRY_H
