#ifndef MAVLINK_TRAITS_H
#define MAVLINK_TRAITS_H

#include <QSharedPointer>

namespace data_source
{
    class AbstractLink;
    using AbstractLinkPtr = AbstractLink*;
} // namespace data_source

namespace domain
{
    class Vehicle;
    using VehiclePtr = QSharedPointer< Vehicle >;

    class Command;
    using CommandPtr = QSharedPointer< Command >;

    class VehicleRegistry;
    using VehicleRegistryPtr = QSharedPointer< VehicleRegistry >;

    class CommandService;
    using CommandServicePtr = QSharedPointer< CommandService >;
}  // namespace domain

#endif // MAVLINK_TRAITS_H
