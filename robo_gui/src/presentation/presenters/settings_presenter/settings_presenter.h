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
        Q_PROPERTY(quint8 quality READ quality WRITE setQuality NOTIFY imageSettingsChanged)
        Q_PROPERTY(quint8 brightness READ brightness WRITE setBrightness NOTIFY imageSettingsChanged)
        Q_PROPERTY(quint8 contrast READ contrast WRITE setContrast NOTIFY imageSettingsChanged)
        Q_PROPERTY(quint8 trackerCode READ trackerCode WRITE setTrackerCode NOTIFY trackerCodeChanged)
        Q_PROPERTY(QString videoSource READ videoSource WRITE setVideoSource NOTIFY videoSourceChanged)

        SettingsPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~SettingsPresenter() override;

        QString videoSource() const;
        quint8 quality() const;
        quint8 brightness() const;
        quint8 contrast() const;
        quint8 trackerCode() const;

    public slots:
        void setVideoSource(const QString& source);
        void setQuality(quint8 quality);
        void setBrightness(quint8 brightness);
        void setContrast(quint8 contrast);
        void setTrackerCode(quint8 code);

        void calibrateGun();
        void calibrateGyro();

        void setEnginePower(int engine, quint8 percent);
        quint8 enginePower(int engine) const;

    signals:
        void videoSourceChanged();
        void imageSettingsChanged();
        void trackerCodeChanged();
        void enginePowerChanged();

    private:
        class Impl;
        Impl* d;
    };
}

#endif //SETTINGS_PRESENTER_H
