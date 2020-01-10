#ifndef VEHICLE_H
#define VEHICLE_H

// Qt
#include <QObject>
#include <QHostAddress>
#include <QSharedPointer>

namespace data_source
{
    class AbstractLink;
}  // namespace data_source

namespace domain
{
    class Vehicle : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(int sysId READ sysId WRITE setSysId NOTIFY vehicleChanged)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY vehicleChanged)
        Q_PROPERTY(data_source::AbstractLink* link READ link WRITE setLink NOTIFY vehicleChanged)
        Q_PROPERTY(Type type READ type WRITE setType NOTIFY vehicleChanged)
        Q_PROPERTY(QString typeString READ typeString NOTIFY vehicleChanged)
        Q_PROPERTY(bool online READ isOnline WRITE setOnline NOTIFY onlineChanged)

    public:
        enum Type: quint8
        {
            UnknownType = 0,
            Auto = 1,

            GCS = 2,

            FixedWing = 10,
            FlyingWing = 11,

            Quadcopter = 20,
            Tricopter = 21,
            Hexcopter = 22,
            Octocopter = 23,

            Helicopter = 30,
            Coaxial = 31,

            Vtol = 40,

            Airship = 60,
            Kite = 61,
            Ornithopter = 62,

            Rover = 70,
            Tank = 71,
        };

        int sysId() const;
        void setSysId(int sysId);

        QString name() const;
        void setName(const QString& name);

        data_source::AbstractLink* link() const;
        void setLink(data_source::AbstractLink* link);

        Type type() const;
        void setType(Type type);
        QString typeString() const;

        bool isOnline() const;
        void setOnline(bool isOnline);

    signals:
        void onlineChanged(bool online);
        void vehicleChanged();

    private:
        int m_sysId = 0;
        QString m_name;
        data_source::AbstractLink* m_link = nullptr;
        Type m_type = UnknownType;

        bool m_online = false;

        Q_ENUM(Type)
    };
}

#endif // VEHICLE_H
