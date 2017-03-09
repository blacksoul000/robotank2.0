#include "status_presenter.h"

#include "robo_model.h"
#include "status_model.h"

#include <QDebug>

using presentation::StatusPresenter;
using domain::StatusModel;

class StatusPresenter::Impl
{
public:
    StatusModel* model = nullptr;
};

StatusPresenter::StatusPresenter(domain::RoboModel *model, QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model->status();

    connect(d->model, &StatusModel::batteryLevelChanged,
            this, &StatusPresenter::batteryLevelChanged);

    connect(d->model, &StatusModel::isChargingChanged,
            this, &StatusPresenter::isChargingChanged);

    connect(d->model, &StatusModel::gamepadBatteryLevelChanged,
            this, &StatusPresenter::gamepadBatteryLevelChanged);

    connect(d->model, &StatusModel::robotBatteryLevelChanged,
            this, &StatusPresenter::robotBatteryLevelChanged);

    connect(d->model, &StatusModel::gunPositionHChanged,
            this, &StatusPresenter::gunPositionHChanged);

    connect(d->model, &StatusModel::gunPositionVChanged,
            this, &StatusPresenter::gunPositionVChanged);

    connect(d->model, &StatusModel::cameraPositionVChanged,
            this, &StatusPresenter::cameraPositionVChanged);

    connect(d->model, &StatusModel::yawChanged, this, &StatusPresenter::yawChanged);
    connect(d->model, &StatusModel::pitchChanged, this, &StatusPresenter::pitchChanged);
    connect(d->model, &StatusModel::rollChanged, this, &StatusPresenter::rollChanged);
}

StatusPresenter::~StatusPresenter()
{
    delete d;
}

int StatusPresenter::batteryLevel() const
{
    return d->model->batteryLevel();
}

bool StatusPresenter::isCharging() const
{
    return d->model->isCharging();
}

int StatusPresenter::gamepadBatteryLevel() const
{
    return d->model->gamepadBatteryLevel();
}

int StatusPresenter::robotBatteryLevel() const
{
    return d->model->robotBatteryLevel();
}

int StatusPresenter::gunPositionH() const
{
    return d->model->gunPositionH();
}

int StatusPresenter::gunPositionV() const
{
    return d->model->gunPositionV();
}

int StatusPresenter::cameraPositionV() const
{
    return d->model->cameraPositionV();
}

qreal StatusPresenter::yaw() const
{
    return d->model->yaw();
}

qreal StatusPresenter::pitch() const
{
    return d->model->pitch();
}

qreal StatusPresenter::roll() const
{
    return d->model->roll();
}
