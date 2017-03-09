#include "chassis_exchanger.h"

#include "chassis_packet.h"
#include "command_packet.h"

#include "robo_model.h"
#include "track_model.h"
#include "status_model.h"
#include "settings_model.h"

#include <QTimer>
#include <QDateTime>
#include <QHostAddress>
#include <QUdpSocket>

using domain::ChassisExchanger;
using domain::RoboModel;

namespace
{
    const quint16 receivePort = 56001;
    const quint16 sendPort = 56002;
    const QHostAddress sendHost = QHostAddress::LocalHost; // FIXME

    constexpr double positionCoef = 360.0 / 32767;

    const int sendInterval = 100; //ms
    const int packetTimeout = 3000; //ms
}

class ChassisExchanger::Impl
{
public:
    QUdpSocket* sender = nullptr;
    QUdpSocket* receiver = nullptr;
    QTimer* timer = nullptr;
    domain::RoboModel* model = nullptr;

    quint8 nextId = 1;
    QHash < qint64, CommandPacket > queue;
    ChassisPacket last;

    QHash < qint64, CommandPacket >::Iterator findQueueId(quint8 id);
    void removeQueueId(quint8 id);
};

ChassisExchanger::ChassisExchanger(domain::RoboModel* model, QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;
    d->sender = new QUdpSocket(this);
    d->receiver = new QUdpSocket(this);
    connect(d->receiver, &QUdpSocket::readyRead, this, &ChassisExchanger::onReadyRead);
    d->receiver->bind(::receivePort);

    d->timer = new QTimer(this);
    d->timer->setInterval(::sendInterval);
    connect(d->timer, &QTimer::timeout, this, &ChassisExchanger::send);

    connect(d->model->track(), &domain::TrackModel::trackRequest, 
            this, &ChassisExchanger::onTrackToggle);
    connect(d->model->settings(), &domain::SettingsModel::qualityChanged,
            this, &ChassisExchanger::onImageSettingsChanged);
    connect(d->model->settings(), &domain::SettingsModel::brightnessChanged,
            this, &ChassisExchanger::onImageSettingsChanged);
    connect(d->model->settings(), &domain::SettingsModel::contrastChanged,
            this, &ChassisExchanger::onImageSettingsChanged);
    connect(d->model->settings(), &domain::SettingsModel::trackerChanged,
            this, &ChassisExchanger::onSelectedTrackerChanged);
    connect(d->model->settings(), &domain::SettingsModel::calibrateGun,
            this, &ChassisExchanger::onCalibrateGun);
    connect(d->model->settings(), &domain::SettingsModel::calibrateCamera,
            this, &ChassisExchanger::onCalibrateCamera);
    connect(d->model->settings(), &domain::SettingsModel::calibrateGyro,
            this, &ChassisExchanger::onCalibrateGyro);
    connect(d->model->settings(), &domain::SettingsModel::enginePowerChanged,
            this, &ChassisExchanger::onEnginePowerChanged);

    d->timer->start();

    this->onRequestConfig();
}


ChassisExchanger::~ChassisExchanger()
{
    delete d;
}

void ChassisExchanger::send()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    for (auto it = d->queue.begin(), end = d->queue.end(); it != end;)
    {
        if (currentTime - it.key() > ::packetTimeout
                && it.value().commandId != CommandPacket::RequestConfig)
        {
            it = d->queue.erase(it);
        }
        else
        {
            d->sender->writeDatagram(it.value().toByteArray(), ::sendHost, ::sendPort);
            ++it;
        }
    }
}

void ChassisExchanger::onReadyRead()
{
    while (d->receiver->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(d->receiver->pendingDatagramSize());
        d->receiver->readDatagram(datagram.data(), datagram.size());
        this->processPacket(datagram);
    }
}

void ChassisExchanger::onImageSettingsChanged()
{
    const auto& s = d->model->settings();
    CommandPacket packet(d->nextId, CommandPacket::ImageSettings);

    QDataStream out(&packet.data, QIODevice::WriteOnly);
    out << s->quality() << s->brightness() << s->contrast();
    d->queue.insert(QDateTime::currentMSecsSinceEpoch(), packet);
    ++d->nextId;
}

void ChassisExchanger::onSelectedTrackerChanged(quint8 code)
{
    CommandPacket packet(d->nextId, CommandPacket::TrackerCode);
    QDataStream out(&packet.data, QIODevice::WriteOnly);
    out << code;
    d->queue.insert(QDateTime::currentMSecsSinceEpoch(), packet);
    ++d->nextId;
}

void ChassisExchanger::onTrackToggle(const QRectF& rect)
{
    CommandPacket packet(d->nextId, CommandPacket::TrackerRect);
    QDataStream out(&packet.data, QIODevice::WriteOnly);
    out << rect;
    d->queue.insert(QDateTime::currentMSecsSinceEpoch(), packet);
    ++d->nextId;
}

void ChassisExchanger::onCalibrateGun()
{
    CommandPacket packet(d->nextId, CommandPacket::CalibrateGun);
    d->queue.insert(QDateTime::currentMSecsSinceEpoch(), packet);
    ++d->nextId;
}

void ChassisExchanger::onCalibrateCamera()
{
    CommandPacket packet(d->nextId, CommandPacket::CalibrateCamera);
    d->queue.insert(QDateTime::currentMSecsSinceEpoch(), packet);
    ++d->nextId;
}

void ChassisExchanger::onCalibrateGyro()
{
    CommandPacket packet(d->nextId, CommandPacket::CalibrateYpr);
    d->queue.insert(QDateTime::currentMSecsSinceEpoch(), packet);
    ++d->nextId;
}

void ChassisExchanger::onEnginePowerChanged()
{
    const auto& s = d->model->settings();
    CommandPacket packet(d->nextId, CommandPacket::EnginePower);
    QDataStream out(&packet.data, QIODevice::WriteOnly);
    out << s->enginePower(domain::SettingsModel::Left)
        << s->enginePower(domain::SettingsModel::Right);
    d->queue.insert(QDateTime::currentMSecsSinceEpoch(), packet);
    ++d->nextId;
}

void ChassisExchanger::onRequestConfig()
{
    CommandPacket packet(d->nextId, CommandPacket::RequestConfig);
    d->queue.insert(QDateTime::currentMSecsSinceEpoch(), packet);
    ++d->nextId;
}

void ChassisExchanger::processPacket(const QByteArray& data)
{
    ChassisPacket packet = ChassisPacket::fromByteArray(data);
    switch (packet.type)
    {
    case PacketType::Response:
    {
        ChassisResponse response = ChassisResponse::fromByteArray(packet.data);
        d->removeQueueId(response.id);
        // notify user about command status?
        break;
    }
    case PacketType::Tmi:
    {
        ChassisTmi tmi = ChassisTmi::fromByteArray(packet.data);
        d->model->status()->setArduinoStatus(tmi.arduinoStatus);
        d->model->status()->setJoyStatus(tmi.joyStatus);

        d->model->track()->setTargetRect(tmi.target);
        d->model->track()->setTracking(tmi.trackerStatus);

        d->model->status()->setGunPositionH(tmi.gunH * ::positionCoef);
        d->model->status()->setGunPositionV(tmi.gunV * ::positionCoef);
        d->model->status()->setCameraPositionV(tmi.cameraV * ::positionCoef);

        d->model->status()->setYaw(tmi.yaw * ::positionCoef);
        d->model->status()->setPitch(tmi.pitch * ::positionCoef);
        d->model->status()->setRoll(tmi.roll * ::positionCoef);

        emit buttonsUpdated(tmi.buttons);
        break;
    }
    case PacketType::Config:
    {
        ChassisConfig config = ChassisConfig::fromByteArray(packet.data);
        d->model->settings()->setQuality(config.quality);
        d->model->settings()->setBrightness(config.brightness);
        d->model->settings()->setContrast(config.contrast);
        d->model->settings()->setEnginePower(SettingsModel::Engine::Left, config.leftEngine);
        d->model->settings()->setEnginePower(SettingsModel::Engine::Right, config.rightEngine);
        d->model->settings()->setTracker(config.selectedTracker);

        d->removeQueueId(config.id);

        break;
    }
    default:
        break;
    }
}

//------------------------------------------------------------------------------------
QHash < qint64, CommandPacket >::Iterator ChassisExchanger::Impl::findQueueId(quint8 id)
{
    return std::find_if(queue.begin(), queue.end(), [id](const CommandPacket& packet)->bool{
        return id == packet.id;
    });
}

void ChassisExchanger::Impl::removeQueueId(quint8 id)
{
    const auto it = this->findQueueId(id);
    if (it == this->queue.end())
    {
        qWarning() << Q_FUNC_INFO << QString("Packet with id = %1 not found in queue.").arg(id);
        return;
    }
    this->queue.erase(it);
}
