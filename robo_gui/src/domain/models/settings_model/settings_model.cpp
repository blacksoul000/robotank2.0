#include "settings_model.h"

#include <QDebug>
#include <QtQml>

using domain::SettingsModel;

class SettingsModel::Impl
{
public:
    quint8 quality = 50;
    quint8 brightness = 75;
    quint8 contrast = 65;
    quint8 tracker = 7; // OpenTLD

    QString videoSource = "rtsp://192.168.1.3:8554/live";

    QMap< SettingsModel::Engine, quint8 > enginePower {{SettingsModel::Engine::Left, 100},
                                                       {SettingsModel::Engine::Right, 100}};
};

SettingsModel::SettingsModel(QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    qmlRegisterUncreatableType< SettingsModel >("Robotank", 1, 0, "Engine", "");
}

SettingsModel::~SettingsModel()
{
    delete d;
}

void SettingsModel::setQuality(quint8 quality)
{
    if (d->quality == quality) return;
    d->quality = quality;
    emit qualityChanged(quality);
}

quint8 SettingsModel::quality() const
{
    return d->quality;
}

void SettingsModel::setTracker(quint8 tracker)
{
    if (d->tracker == tracker) return;
    d->tracker = tracker;
    emit trackerChanged(tracker);
}

quint8 SettingsModel::tracker() const
{
    return d->tracker;
}

void SettingsModel::setBrightness(quint8 brightness)
{
    if (d->brightness == brightness) return;
    d->brightness = brightness;
    emit brightnessChanged(brightness);
}

quint8 SettingsModel::brightness() const
{
    return d->brightness;
}

void SettingsModel::setContrast(quint8 contrast)
{
    if (d->contrast == contrast) return;
    d->contrast = contrast;
    emit contrastChanged(contrast);
}

quint8 SettingsModel::contrast() const
{
    return d->contrast;
}

void SettingsModel::setEnginePower(SettingsModel::Engine engine, quint8 percent)
{
    d->enginePower[engine] = percent;
    emit enginePowerChanged();
}

quint8 SettingsModel::enginePower(SettingsModel::Engine engine) const
{
    return d->enginePower.value(engine);
}

void SettingsModel::setVideoSource(const QString& source)
{
    if (d->videoSource == source) return;
    d->videoSource = source;
    emit videoSourceChanged(source);
}

QString SettingsModel::videoSource() const
{
    return d->videoSource;
}
