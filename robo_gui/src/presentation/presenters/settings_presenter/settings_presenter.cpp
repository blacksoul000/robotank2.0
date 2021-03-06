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

    connect(d->model->settings(), &SettingsModel::videoSourceChanged,
            this, &SettingsPresenter::videoSourceChanged);

    connect(d->model->settings(), &SettingsModel::imageSettingsChanged,
            this, &SettingsPresenter::imageSettingsChanged);

    connect(d->model->settings(), &SettingsModel::trackerChanged,
            this, &SettingsPresenter::trackerCodeChanged);

    connect(d->model->settings(), &SettingsModel::enginePowerChanged,
            this, &SettingsPresenter::enginePowerChanged);

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

void SettingsPresenter::setVideoSource(const QString& source)
{
    d->model->settings()->setVideoSource(source);
}

QString SettingsPresenter::videoSource() const
{
    return d->model->settings()->videoSource();
}
