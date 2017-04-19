#include "settings_presenter.h"

#include "robo_model.h"
#include "settings_model.h"

#include <QtQml>
#include <QRect>
#include <QDebug>

using presentation::SettingsPresenter;
using domain::SettingsModel;

class SettingsPresenter::Impl
{
public:
    domain::RoboModel* model = nullptr;
};

SettingsPresenter::SettingsPresenter(domain::RoboModel *model, QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;

    connect(d->model->settings(), &SettingsModel::qualityChanged,
            this, &SettingsPresenter::qualityChanged);
    connect(d->model->settings(), &SettingsModel::brightnessChanged,
            this, &SettingsPresenter::brightnessChanged);
    connect(d->model->settings(), &SettingsModel::contrastChanged,
            this, &SettingsPresenter::contrastChanged);

    connect(d->model->settings(), &SettingsModel::trackerChanged,
            this, &SettingsPresenter::trackerCodeChanged);

    connect(d->model->settings(), &SettingsModel::enginePowerChanged,
            this, &SettingsPresenter::enginePowerChanged);

    connect(d->model->settings(), &SettingsModel::streamProtocolChanged,
            this, &SettingsPresenter::streamProtocolChanged);
    connect(d->model->settings(), &SettingsModel::streamHostChanged,
            this, &SettingsPresenter::streamHostChanged);
    connect(d->model->settings(), &SettingsModel::streamPortChanged,
            this, &SettingsPresenter::streamPortChanged);
    connect(d->model->settings(), &SettingsModel::streamNameChanged,
            this, &SettingsPresenter::streamNameChanged);
}

SettingsPresenter::~SettingsPresenter()
{
    delete d;
}

quint8 SettingsPresenter::quality() const
{
    return d->model->settings()->quality();
}

quint8 SettingsPresenter::brightness() const
{
    return d->model->settings()->brightness();
}

quint8 SettingsPresenter::contrast() const
{
    return d->model->settings()->contrast();
}

quint8 SettingsPresenter::trackerCode() const
{
    return d->model->settings()->tracker();
}

void SettingsPresenter::setQuality(quint8 quality)
{
    d->model->settings()->setQuality(quality);
}

void SettingsPresenter::setBrightness(quint8 brightness)
{
    d->model->settings()->setBrightness(brightness);
}

void SettingsPresenter::setContrast(quint8 contrast)
{
    d->model->settings()->setContrast(contrast);
}

void SettingsPresenter::setTrackerCode(quint8 tracker)
{
    d->model->settings()->setTracker(tracker);
}

void SettingsPresenter::calibrateGun()
{
    d->model->settings()->calibrateGun();
}

void SettingsPresenter::calibrateCamera()
{
    d->model->settings()->calibrateCamera();
}

void SettingsPresenter::calibrateGyro()
{
    d->model->settings()->calibrateGyro();
}

void SettingsPresenter::setEnginePower(int engine, quint8 percent)
{
    d->model->settings()->setEnginePower(static_cast<SettingsModel::Engine>(engine), percent);
}

quint8 SettingsPresenter::enginePower(int engine) const
{
    return d->model->settings()->enginePower(static_cast<SettingsModel::Engine>(engine));
}

void SettingsPresenter::setStreamProtocol(const QString& protocol)
{
    d->model->settings()->setStreamProtocol(protocol);
}

QString SettingsPresenter::streamProtocol() const
{
    return d->model->settings()->streamProtocol();
}

void SettingsPresenter::setStreamHost(const QString& host)
{
    d->model->settings()->setStreamHost(host);
}

QString SettingsPresenter::streamHost() const
{
    return d->model->settings()->streamHost();
}

void SettingsPresenter::setStreamPort(const QString& port)
{
    d->model->settings()->setStreamPort(port);
}

QString SettingsPresenter::streamPort() const
{
    return d->model->settings()->streamPort();
}

void SettingsPresenter::setStreamName(const QString& name)
{
    d->model->settings()->setStreamName(name);
}

QString SettingsPresenter::streamName() const
{
    return d->model->settings()->streamName();
}
