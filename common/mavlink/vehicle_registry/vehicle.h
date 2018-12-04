#ifndef VEHICLE_H
#define VEHICLE_H

// Qt
#include <QHostAddress>
#include <QSharedPointer>

namespace data_source
{
    class AbstractLink;
}  // namespace data_source

namespace domain
{
    class Vehicle
    {
        Q_GADGET

        Q_PROPERTY(int sysId READ sysId WRITE setSysId)
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(data_source::AbstractLink* link READ link WRITE setLink)
        Q_PROPERTY(Type type READ type WRITE setType)
        Q_PROPERTY(bool online READ isOnline WRITE setOnline)

    public:
        enum Type: quint8
        {
            UnknownType = 0,
            Auto = 1,

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
            Ornithopter = 62
        };

        int sysId() const;
        void setSysId(int sysId);

        QString name() const;
        void setName(const QString& name);

        data_source::AbstractLink* link() const;
        void setLink(data_source::AbstractLink* link);

        Type type() const;
        void setType(Type type);

        bool isOnline() const;
        void setOnline(bool isOnline);

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
