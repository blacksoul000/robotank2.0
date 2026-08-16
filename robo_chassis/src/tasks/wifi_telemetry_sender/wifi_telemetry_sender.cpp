#include "wifi_telemetry_sender.h"
#include <QDebug>
#include <QJsonDocument>

class WifiTelemetrySender::Impl
{
public:
    QTcpSocket* socket = nullptr;
    QString host = "127.0.0.1";
    quint16 port = 5555;
    QTimer* timer = nullptr;
    
    // Телеметрия
    double roll = 0.0;
    double pitch = 0.0;
    double turretAngle = 0.0;
    int batteryLevel = 0;
};

WifiTelemetrySender::WifiTelemetrySender(const QString& host, quint16 port) : d(new Impl())
{
    d->host = host;
    d->port = port;
    d->socket = new QTcpSocket(this);
    d->timer = new QTimer(this);
    d->timer->setInterval(100); // 10 Гц
    
    connect(d->socket, &QTcpSocket::connected, this, &WifiTelemetrySender::onConnected);
    connect(d->socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &WifiTelemetrySender::onError);
    connect(d->timer, &QTimer::timeout, this, &WifiTelemetrySender::sendTelemetry);
}

WifiTelemetrySender::~WifiTelemetrySender()
{
    delete d;
}

void WifiTelemetrySender::execute()
{
    qDebug() << "WifiTelemetrySender connecting to" << d->host << ":" << d->port;
    d->socket->connectToHost(d->host, d->port);
}

void WifiTelemetrySender::setTelemetryData(double roll, double pitch, double turretAngle, int batteryLevel)
{
    d->roll = roll;
    d->pitch = pitch;
    d->turretAngle = turretAngle;
    d->batteryLevel = batteryLevel;
}

void WifiTelemetrySender::onConnected()
{
    qDebug() << "Connected to Python bridge. Starting telemetry transmission.";
    d->timer->start();
}

void WifiTelemetrySender::sendTelemetry()
{
    if (d->socket->state() != QTcpSocket::ConnectedState) {
        return;
    }

    QJsonObject telemetry;
    telemetry["type"] = "TELEMETRY";
    telemetry["roll"] = d->roll;
    telemetry["pitch"] = d->pitch;
    telemetry["turret_angle"] = d->turretAngle;
    telemetry["battery"] = d->batteryLevel;

    QJsonDocument doc(telemetry);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    
    d->socket->write(data);
}

void WifiTelemetrySender::onError(QAbstractSocket::SocketError error)
{
    qWarning() << "TcpSocket error:" << d->socket->errorString();
    // Попытка переподключения через 2 секунды
    QTimer::singleShot(2000, this, [this]() {
        if (d->socket->state() != QTcpSocket::ConnectedState) {
            d->socket->connectToHost(d->host, d->port);
        }
    });
}
