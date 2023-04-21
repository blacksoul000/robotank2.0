#ifndef VEHICLE_VIEW_H
#define VEHICLE_VIEW_H

#include "vehicle.h"
#include "mavlink_traits.h"

// Qt
#include <QObject>

namespace presentation
{
    class VehicleView : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(int sysId READ sysId NOTIFY sysIdChanged)
        Q_PROPERTY(QString name READ name NOTIFY nameChanged)
        Q_PROPERTY(QString type READ type NOTIFY typeChanged)
        Q_PROPERTY(bool online READ isOnline NOTIFY onlineChanged)

    public:
        VehicleView(const domain::VehiclePtr& vehicle);

        int sysId() const;
        QString name() const;
        QString type() const;
        bool isOnline() const;

        const domain::VehiclePtr& vehicle() const;

    signals:
        void sysIdChanged();
        void nameChanged();
        void typeChanged();
        void onlineChanged();

    private:
        QString typeString(domain::Vehicle::Type type) const;

    private:
        int m_sysId = 0;
        QString m_name;
        QString m_type;

        bool m_online = false;
        domain::VehiclePtr m_vehicle;
    };
}

#endif // VEHICLE_VIEW_H
