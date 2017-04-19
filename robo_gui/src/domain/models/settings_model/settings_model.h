#ifndef SETTINGS_MODEL_H
#define SETTINGS_MODEL_H

#include <QObject>

namespace domain
{
    class SettingsModel : public QObject
    {
        Q_OBJECT
    public:
        enum Engine { Left, Right };
        Q_ENUMS(Engine)

        SettingsModel(QObject* parent = nullptr);
        ~SettingsModel();

        void setQuality(quint8 quality);
        quint8 quality() const;

        void setTracker(quint8 tracker);
        quint8 tracker() const;

        void setBrightness(quint8 brightness);
        quint8 brightness() const;

        void setContrast(quint8 contrast);
        quint8 contrast() const;

        void setEnginePower(Engine engine, quint8 percent);
        quint8 enginePower(Engine engine) const;

        void setStreamProtocol(const QString& protocol);
        QString streamProtocol() const;

        void setStreamHost(const QString& host);
        QString streamHost() const;

        void setStreamPort(const QString& port);
        QString streamPort() const;

        void setStreamName(const QString& name);
        QString streamName() const;

    signals:
        void qualityChanged(quint8 quality);
        void brightnessChanged(quint8 brightness);
        void contrastChanged(quint8 contrast);
        void trackerChanged(quint8 tracker);

        void calibrateGun();
        void calibrateCamera();
        void calibrateGyro();

        void enginePowerChanged();

        void streamProtocolChanged(const QString& protocol);
        void streamHostChanged(const QString& host);
        void streamPortChanged(const QString& port);
        void streamNameChanged(const QString& name);

    private:
        class Impl;
        Impl* d;
    };
}

#endif //SETTINGS_MODEL_H
