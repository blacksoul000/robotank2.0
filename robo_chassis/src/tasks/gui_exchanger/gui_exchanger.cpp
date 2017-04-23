#include "gui_exchanger.h"

//msgs
#include "image_settings.h"
#include "point3d.h"
#include "empty.h"

#include "pub_sub.h"
#include "chassis_packet.h"
#include "command_packet.h"

#include <QPoint>
#include <QUdpSocket>
#include <QHostAddress>
#include <QDataStream>

namespace
{
    const quint16 sendPort = 56001;
    const quint16 receivePort = 56002;
    const QHostAddress sendHost = QHostAddress::LocalHost; // FIXME

    constexpr double positionCoef = 360.0 / 32767;
}

class GuiExchanger::Impl
{
public:
    Publisher< QRectF >* trackRectP = nullptr;
    Publisher< quint8 >* trackSelectorP = nullptr;
    Publisher< ImageSettings >* imageSettingsP = nullptr;
    Publisher< Empty >* calibrateGunP = nullptr;
    Publisher< Empty >* calibrateCameraP = nullptr;
    Publisher< Empty >* calibrateYprP = nullptr;
    Publisher< QPoint >* enginePowerP = nullptr;
    Publisher< QString >* videoSourceP = nullptr;

    QUdpSocket* sender = nullptr;
    QUdpSocket* receiver = nullptr;

    ChassisTmi chassis;
    CommandPacket lastCommand;

    ImageSettings imageSettings;
    QPoint enginePower;
    quint8 selectedTracker;

    QString videoSource;

    void processPacket(const CommandPacket& packet);
};

GuiExchanger::GuiExchanger() :
    ITask(),
    d(new Impl)
{
    d->trackRectP = PubSub::instance()->advertise< QRectF >("tracker/toggle");
    d->trackSelectorP = PubSub::instance()->advertise< quint8 >("tracker/selector");
    d->imageSettingsP = PubSub::instance()->advertise< ImageSettings >("camera/settings");
    d->calibrateGunP = PubSub::instance()->advertise< Empty >("gun/calibrate");
    d->calibrateCameraP = PubSub::instance()->advertise< Empty >("camera/calibrate");
    d->calibrateYprP = PubSub::instance()->advertise< Empty >("ypr/calibrate");
    d->enginePowerP = PubSub::instance()->advertise< QPoint >("core/enginePower");
    d->videoSourceP = PubSub::instance()->advertise< QString >("camera/source");

    PubSub::instance()->subscribe("gun/position", &GuiExchanger::onGunPosition, this);
    PubSub::instance()->subscribe("camera/position", &GuiExchanger::onCameraPosition, this);
    PubSub::instance()->subscribe("robo/ypr", &GuiExchanger::onYpr, this);
    PubSub::instance()->subscribe("arduino/status", &GuiExchanger::onArduinoStatus, this);
    PubSub::instance()->subscribe("joy/buttons", &GuiExchanger::onJoyButtons, this);
    PubSub::instance()->subscribe("joy/status", &GuiExchanger::onJoyStatus, this);
    PubSub::instance()->subscribe("tracker/target", &GuiExchanger::onTrackerTarget, this);
    PubSub::instance()->subscribe("tracker/status", &GuiExchanger::onTrackerStatus, this);

    PubSub::instance()->subscribe("core/enginePower", &GuiExchanger::onEnginePowerChanged, this);
    PubSub::instance()->subscribe("camera/settings", &GuiExchanger::onImageSettingsChanged, this);
    PubSub::instance()->subscribe("tracker/selector", &GuiExchanger::onSwitchTrackerRequest, this);

    PubSub::instance()->subscribe("camera/source", &GuiExchanger::onVideoSourceChanged, this);
}

GuiExchanger::~GuiExchanger()
{
    delete d->trackRectP;
    delete d->trackSelectorP;
    delete d->imageSettingsP;
    delete d->calibrateGunP;
    delete d->calibrateCameraP;
    delete d->calibrateYprP;
    delete d->enginePowerP;
    delete d->videoSourceP;
    delete d;
}

void GuiExchanger::start()
{
    d->receiver = new QUdpSocket(this);
    connect(d->receiver, &QUdpSocket::readyRead, this, &GuiExchanger::onReadyRead);
    d->receiver->bind(::receivePort);

    d->sender = new QUdpSocket(this);
}

void GuiExchanger::execute()
{
    d->sender->writeDatagram(ChassisPacket(d->chassis).toByteArray(), ::sendHost, ::sendPort);
}

void GuiExchanger::onReadyRead()
{
    while (d->receiver->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(d->receiver->pendingDatagramSize());
        d->receiver->readDatagram(datagram.data(), datagram.size());
        d->processPacket(CommandPacket::fromByteArray(datagram));
    }
}

void GuiExchanger::onGunPosition(const QPoint& position)
{
    d->chassis.gunH = position.x();
    d->chassis.gunV = position.y();
}

void GuiExchanger::onCameraPosition(const QPoint& position)
{
    d->chassis.cameraH = position.x();
    d->chassis.cameraV = position.y();
}

void GuiExchanger::onYpr(const Point3D& ypr)
{
    d->chassis.yaw = ypr.x;
    d->chassis.pitch = ypr.y;
    d->chassis.roll = ypr.z;
}

void GuiExchanger::onArduinoStatus(const bool& status)
{
    d->chassis.arduinoStatus = status;
}

void GuiExchanger::onJoyButtons(const quint16& buttons)
{
    d->chassis.buttons = buttons;
}

void GuiExchanger::onJoyStatus(const bool& status)
{
    d->chassis.joyStatus = status;
}

void GuiExchanger::onTrackerTarget(const QRectF& target)
{
    d->chassis.target = target;
}

void GuiExchanger::onTrackerStatus(const bool& status)
{
    d->chassis.trackerStatus = status;
}

void GuiExchanger::onImageSettingsChanged(const ImageSettings& settings)
{
    d->imageSettings = settings;
}

void GuiExchanger::onEnginePowerChanged(const QPoint& enginePower)
{
    d->enginePower = enginePower;
}

void GuiExchanger::onSwitchTrackerRequest(const quint8& code)
{
    d->selectedTracker = code;
}

void GuiExchanger::onVideoSourceChanged(const QString& source)
{
    d->videoSource = source;
}

//------------------------------------------------------------------------------------
void GuiExchanger::Impl::processPacket(const CommandPacket& packet)
{
    switch (packet.commandId)
    {
    case CommandPacket::CommandId::CalibrateGun:
    {
        calibrateGunP->publish(Empty());
        break;
    }
    case CommandPacket::CommandId::CalibrateCamera:
    {
        calibrateCameraP->publish(Empty());
        break;
    }
    case CommandPacket::CommandId::CalibrateYpr:
    {
        calibrateYprP->publish(Empty());
        break;
    }
    case CommandPacket::CommandId::ImageSettings:
    {
        QDataStream in(packet.data);
        quint8 quality;
        quint8 brightness;
        quint8 contrast;
        in >> quality >> brightness >> contrast;
        if (imageSettings.quality != quality || imageSettings.brightness != brightness
                || imageSettings.contrast != contrast)
        {
            imageSettings = ImageSettings {quality, brightness, contrast};
            imageSettingsP->publish(imageSettings);
        }
        break;
    }
    case CommandPacket::CommandId::EnginePower:
    {
        QDataStream in(packet.data);
        quint8 left;
        quint8 right;
        in >> left >> right;
        if (enginePower.x() != left || enginePower.y() != right)
        {
            enginePower = QPoint(left, right);
            enginePowerP->publish(enginePower);
        }
        break;
    }
    case CommandPacket::CommandId::TrackerCode:
    {
        QDataStream in(packet.data);
        quint8 code;
        in >> code;
        if (selectedTracker != code)
        {
            selectedTracker = code;
            trackSelectorP->publish(selectedTracker);
        }
        break;
    }
    case CommandPacket::CommandId::TrackerRect:
    {
        QDataStream in(packet.data);
        QRectF rect;
        in >> rect;
        trackRectP->publish(rect);
        break;
    }
    case CommandPacket::CommandId::VideoSource:
    {
        QDataStream in(packet.data);
        QString source;
        in >> source;
        if (videoSource != source)
        {
            videoSource = source;
            videoSourceP->publish(videoSource);
        }
        break;
    }
    case CommandPacket::CommandId::RequestConfig:
    {
        ChassisConfig config;
        config.id = packet.id;
        config.quality = imageSettings.quality;
        config.brightness = imageSettings.brightness;
        config.contrast = imageSettings.contrast;
        config.leftEngine = enginePower.x();
        config.rightEngine = enginePower.y();
        config.selectedTracker = selectedTracker;
        config.videoSource = videoSource;
        sender->writeDatagram(ChassisPacket(config).toByteArray(), ::sendHost, ::sendPort);
        return;
    }
    default:
        break;
    }

    ChassisResponse response;
    response.id = packet.id;
    response.status = 0xAA;
    sender->writeDatagram(ChassisPacket(response).toByteArray(), ::sendHost, ::sendPort);
}
