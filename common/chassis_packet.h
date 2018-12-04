#ifndef CHASSIS_PACKET_H
#define CHASSIS_PACKET_H

#include <QRectF>
#include <QVector>
#include <QDataStream>

#pragma pack(push, 1)

enum PacketType : uint8_t
{
    Response,
    Tmi,
    Config,
    Bluetooth
};

struct DeviceInfo
{
    QString address;
    QString name;
    bool isPaired;
};

inline QDataStream& operator<<(QDataStream& out, const DeviceInfo& info)
{
    out << info.address << info.name << info.isPaired;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, DeviceInfo& info)
{
    in >> info.address >> info.name >> info.isPaired;
    return in;
}

struct ChassisResponse
{
    uint8_t id = 0;
    uint8_t status = 0;

    QByteArray toByteArray() const
    {
        return QByteArray(reinterpret_cast< const char* >(this), sizeof(ChassisResponse));
    }

    static ChassisResponse fromByteArray(const QByteArray& data)
    {
        return *reinterpret_cast<const ChassisResponse* >(data.data());
    }
};

struct ChassisConfig
{
    uint8_t id = 0;
    uint8_t quality = 0;
    uint8_t contrast = 0;
    uint8_t brightness = 0;
    uint8_t leftEngine = 0;
    uint8_t rightEngine = 0;
    uint8_t selectedTracker = 0;
    QString videoSource;

    QByteArray toByteArray() const
    {
        QByteArray result;
        QDataStream out(&result, QIODevice::WriteOnly);
        out << id << quality << contrast << brightness
            << leftEngine << rightEngine
            << selectedTracker << videoSource;
        return result;
    }

    static ChassisConfig fromByteArray(const QByteArray& data)
    {
        ChassisConfig cfg;
        QDataStream in(data);
        in >> cfg.id >> cfg.quality >> cfg.contrast >> cfg.brightness
            >> cfg.leftEngine >> cfg.rightEngine
            >> cfg.selectedTracker >> cfg.videoSource;
        return cfg;
    }
};

struct ChassisTmi
{
    const uint8_t type = PacketType::Tmi;

    int8_t arduinoStatus: 1;
    int8_t joyStatus: 1;
    int8_t trackerStatus: 1;
    int8_t joyCharging: 1;
    int8_t headlight: 1;
    int8_t pointer: 1;
    int8_t reserve: 2;

    int16_t gunH = 0;
    int16_t gunV = 0;

    int16_t yaw = 0;
    int16_t pitch = 0;
    int16_t roll = 0;

    QRectF target;
    int16_t buttons = 0;
    uint8_t joyCapacity = 0;
    uint16_t voltage = 0;

    QByteArray toByteArray() const
    {
        return QByteArray(reinterpret_cast< const char* >(this), sizeof(ChassisTmi));
    }

    static ChassisTmi fromByteArray(const QByteArray& data)
    {
        return *reinterpret_cast<const ChassisTmi* >(data.data());
    }
};

struct ChassisBluetoothStatus
{
    uint8_t id = 0;
    struct Status
    {
        uint8_t scanStatus: 1;
        uint8_t reserve: 7;
    } status;

    QVector< DeviceInfo > devices;

    QByteArray toByteArray() const
    {
        QByteArray result;
        QDataStream out(&result, QIODevice::WriteOnly);
        out << id;
        out.writeRawData(reinterpret_cast<const char*>(&status), sizeof(Status));
        out << devices;
        return result;
    }

    static ChassisBluetoothStatus fromByteArray(const QByteArray& data)
    {
        ChassisBluetoothStatus bt;
        QDataStream in(data);
        in >> bt.id;
        in.readRawData(reinterpret_cast<char*>(&bt.status), sizeof(Status));
        in >> bt.devices;
        return bt;
    }
};

struct ChassisPacket
{
    ChassisPacket() = default;
    explicit ChassisPacket(const ChassisResponse& response) :
        type(PacketType::Response),
        data(response.toByteArray())
    {}
    explicit ChassisPacket(const ChassisTmi& tmi) :
        type(PacketType::Tmi),
        data(tmi.toByteArray())
    {}
    explicit ChassisPacket(const ChassisConfig& config) :
        type(PacketType::Config),
        data(config.toByteArray())
    {}
    explicit ChassisPacket(const ChassisBluetoothStatus& data) :
        type(PacketType::Bluetooth),
        data(data.toByteArray())
    {}

    uint8_t type;
    QByteArray data;

    QByteArray toByteArray() const
    {
        QByteArray res;
        QDataStream out(&res, QIODevice::WriteOnly);
        out << type << quint16(data.length());
        out.writeRawData(data.data(), data.length());
        return res;
    }

    static ChassisPacket fromByteArray(const QByteArray& ba)
    {
        ChassisPacket res;

        QDataStream in(ba);
        in >> res.type;
        quint16 size;
        in >> size;
        res.data.resize(size);
        in.readRawData(res.data.data(), size);
        return res;
    }
};

#pragma pack(pop)

#endif // CHASSIS_PACKET_H
