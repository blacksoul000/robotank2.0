#ifndef MAVLINK_TRAITS_H
#define MAVLINK_TRAITS_H

#include <QSharedPointer>

namespace data_source
{
} // namespace data_source

namespace domain
{
    class Vehicle;
    using VehiclePtr = QSharedPointer< Vehicle >;

    class VehicleRegistry;
    using VehicleRegistryPtr = QSharedPointer< VehicleRegistry >;
}  // namespace domain

#endif // MAVLINK_TRAITS_H
