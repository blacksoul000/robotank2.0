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

    signals:
        void qualityChanged(quint8 quality);
        void brightnessChanged(quint8 brightness);
        void contrastChanged(quint8 contrast);
        void trackerCodeChanged(quint8 tracker);
        void enginePowerChanged();

    private:
        class Impl;
        Impl* d;
    };
}

#endif //SETTINGS_PRESENTER_H
