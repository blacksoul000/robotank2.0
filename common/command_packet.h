#ifndef COMMAND_PACKET_H
#define COMMAND_PACKET_H

#include <QRectF>
#include <QDataStream>


#pragma pack(push, 1)

struct CommandPacket
{
    enum CommandId: uint8_t
    {
        None,
        CalibrateGun,
        CalibrateCamera,
        CalibrateYpr,
        ImageSettings,
        EnginePower,
        TrackerCode,
        TrackerRect,
        VideoSource,
        RequestConfig
    };

    CommandPacket() = default;
    CommandPacket(uint8_t id, CommandId commandId) : id(id), commandId(commandId){}
    uint8_t id;
    uint8_t commandId = CommandId::None;
    QByteArray data;

    QByteArray toByteArray() const
    {
        QByteArray res;
        QDataStream out(&res, QIODevice::WriteOnly);
        out << id << commandId << uint8_t(data.length());
        out.writeRawData(data.data(), data.length());
        return res;
    }

    static CommandPacket fromByteArray(const QByteArray& data)
    {
        CommandPacket packet;
        QDataStream in(data);
        in >> packet.id;
        in >> packet.commandId;
        int8_t size;
        in >> size;
        packet.data.resize(size);
        in.readRawData(packet.data.data(), size);
        return packet;
    }
};

#pragma pack(pop)

#endif // COMMAND_PACKET_H
