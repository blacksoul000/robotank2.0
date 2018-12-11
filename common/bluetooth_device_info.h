#ifndef BLUETOOTH_DEVICE_INFO_H
#define BLUETOOTH_DEVICE_INFO_H

#include <QDataStream>

#pragma pack(push, 1)

struct BluetoothDeviceInfo
{
    QString address;
    QString name;
    bool isPaired;
};

inline QDataStream& operator<<(QDataStream& out, const BluetoothDeviceInfo& info)
{
    out << info.address << info.name << info.isPaired;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, BluetoothDeviceInfo& info)
{
    in >> info.address >> info.name >> info.isPaired;
    return in;
}

#pragma pack(pop)

#endif // BLUETOOTH_DEVICE_INFO_H
