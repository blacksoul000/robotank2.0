#ifndef WIFI_TELEMETRY_SENDER_H
#define WIFI_TELEMETRY_SENDER_H

#include "i_task.h"
#include <QTcpSocket>
#include <QJsonObject>
#include <QTimer>

class WifiTelemetrySender : public ITask
{
    Q_OBJECT
public:
    explicit WifiTelemetrySender(const QString& host = "127.0.0.1", quint16 port = 5555);
    ~WifiTelemetrySender();

    void execute() override;
    
    void setTelemetryData(double roll, double pitch, double turretAngle, int batteryLevel);

private slots:
    void onConnected();
    void sendTelemetry();
    void onError(QAbstractSocket::SocketError error);

private:
    class Impl;
    Impl* d;
};

#endif // WIFI_TELEMETRY_SENDER_H
