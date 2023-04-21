#include "settings_model.h"

#include <QMutex>
#include <QMutexLocker>
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
    QMutex mutex;

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
    QMutexLocker locker(&d->mutex);
    if (d->quality == quality) return;
    d->quality = quality;
    locker.unlock();

    emit qualityChanged();
}

quint8 SettingsModel::quality() const
{
    QMutexLocker locker(&d->mutex);
    return d->quality;
}

void SettingsModel::setTracker(quint8 tracker)
{
    QMutexLocker locker(&d->mutex);
    if (d->tracker == tracker) return;
    d->tracker = tracker;
    locker.unlock();

    emit trackerChanged();
}

quint8 SettingsModel::tracker() const
{
    QMutexLocker locker(&d->mutex);
    return d->tracker;
}

void SettingsModel::setBrightness(quint8 brightness)
{
    QMutexLocker locker(&d->mutex);
    if (d->brightness == brightness) return;
    d->brightness = brightness;
    locker.unlock();

    emit brightnessChanged();
}

quint8 SettingsModel::brightness() const
{
    QMutexLocker locker(&d->mutex);
    return d->brightness;
}

void SettingsModel::setContrast(quint8 contrast)
{
    QMutexLocker locker(&d->mutex);
    if (d->contrast == contrast) return;
    d->contrast = contrast;
    locker.unlock();

    emit contrastChanged();
}

quint8 SettingsModel::contrast() const
{
    QMutexLocker locker(&d->mutex);
    return d->contrast;
}

void SettingsModel::setEnginePower(SettingsModel::Engine engine, quint8 percent)
{
    QMutexLocker locker(&d->mutex);
    d->enginePower[engine] = percent;
    locker.unlock();

    emit enginePowerChanged();
}

quint8 SettingsModel::enginePower(SettingsModel::Engine engine) const
{
    QMutexLocker locker(&d->mutex);
    return d->enginePower.value(engine);
}

void SettingsModel::setVideoSource(const QString& source)
{
    QMutexLocker locker(&d->mutex);
    if (d->videoSource == source) return;
    d->videoSource = source;
    locker.unlock();

    emit videoSourceChanged();
}

QString SettingsModel::videoSource() const
{
    QMutexLocker locker(&d->mutex);
    return d->videoSource;
}

void SettingsModel::setImageSettings(quint8 quality, quint8 brightness, quint8 contrast)
{
    QMutexLocker locker(&d->mutex);
    d->quality = quality;
    d->brightness = brightness;
    d->contrast = contrast;
    locker.unlock();

    emit imageSettingsChanged();
}

void SettingsModel::setEnginePower(quint8 left, quint8 right)
{
    QMutexLocker locker(&d->mutex);
    d->enginePower[SettingsModel::Engine::Left] = left;
    d->enginePower[SettingsModel::Engine::Right] = right;
    locker.unlock();

    emit enginePowerChanged();
}
