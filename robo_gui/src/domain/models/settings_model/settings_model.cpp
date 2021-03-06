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

    QString videoSource;

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
    emit qualityChanged();
}

quint8 SettingsModel::quality() const
{
    return d->quality;
}

void SettingsModel::setTracker(quint8 tracker)
{
    if (d->tracker == tracker) return;
    d->tracker = tracker;
    emit trackerChanged();
}

quint8 SettingsModel::tracker() const
{
    return d->tracker;
}

void SettingsModel::setBrightness(quint8 brightness)
{
    if (d->brightness == brightness) return;
    d->brightness = brightness;
    emit brightnessChanged();
}

quint8 SettingsModel::brightness() const
{
    return d->brightness;
}

void SettingsModel::setContrast(quint8 contrast)
{
    if (d->contrast == contrast) return;
    d->contrast = contrast;
    emit contrastChanged();
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
    emit videoSourceChanged();
}

QString SettingsModel::videoSource() const
{
    return d->videoSource;
}

void SettingsModel::setImageSettings(quint8 quality, quint8 brightness, quint8 contrast)
{
    d->quality = quality;
    d->brightness = brightness;
    d->contrast = contrast;

    emit imageSettingsChanged();
}

void SettingsModel::setEnginePower(quint8 left, quint8 right)
{
    d->enginePower[SettingsModel::Engine::Left] = left;
    d->enginePower[SettingsModel::Engine::Right] = right;
    emit enginePowerChanged();
}
