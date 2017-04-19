#ifndef SETTINGS_PRESENTER_H
#define SETTINGS_PRESENTER_H

#include <QObject>

namespace domain
{
    class RoboModel;
}

namespace presentation
{
    class SettingsPresenter : public QObject
    {
        Q_OBJECT
    public:
        Q_PROPERTY(quint8 quality READ quality WRITE setQuality NOTIFY qualityChanged)
        Q_PROPERTY(quint8 brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
        Q_PROPERTY(quint8 contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
        Q_PROPERTY(quint8 trackerCode READ trackerCode WRITE setTrackerCode NOTIFY trackerCodeChanged)
        Q_PROPERTY(QString streamProtocol READ streamProtocol WRITE setStreamProtocol NOTIFY streamProtocolChanged)
        Q_PROPERTY(QString streamHost READ streamHost WRITE setStreamHost NOTIFY streamHostChanged)
        Q_PROPERTY(QString streamPort READ streamPort WRITE setStreamPort NOTIFY streamPortChanged)
        Q_PROPERTY(QString streamName READ streamName WRITE setStreamName NOTIFY streamNameChanged)

        SettingsPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~SettingsPresenter() override;

        quint8 quality() const;
        quint8 brightness() const;
        quint8 contrast() const;
        quint8 trackerCode() const;

    public slots:
        void setQuality(quint8 quality);
        void setBrightness(quint8 brightness);
        void setContrast(quint8 contrast);
        void setTrackerCode(quint8 code);

        void calibrateGun();
        void calibrateCamera();
        void calibrateGyro();

        void setEnginePower(int engine, quint8 percent);
        quint8 enginePower(int engine) const;

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
        void trackerCodeChanged(quint8 tracker);
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

#endif //SETTINGS_PRESENTER_H
