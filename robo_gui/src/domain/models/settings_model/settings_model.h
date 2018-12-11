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

        void setVideoSource(const QString& source);
        QString videoSource() const;

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

        void setImageSettings(quint8 quality, quint8 brightness, quint8 contrast);
        void setEnginePower(quint8 left, quint8 right);

    signals:
        void videoSourceChanged();
        void qualityChanged();
        void brightnessChanged();
        void contrastChanged();
        void trackerChanged();

        void calibrateGun();
        void calibrateGyro();

        void enginePowerChanged();
        void imageSettingsChanged();

    private:
        class Impl;
        Impl* d;
    };
}

#endif //SETTINGS_MODEL_H
