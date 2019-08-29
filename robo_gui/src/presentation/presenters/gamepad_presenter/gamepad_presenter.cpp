#include "gamepad_presenter.h"

#include "robo_model.h"
#include "gamepad_model.h"

#include <QDebug>

using presentation::GamepadPresenter;
using domain::GamepadModel;

class GamepadPresenter::Impl
{
public:
	GamepadModel* model = nullptr;
};

GamepadPresenter::GamepadPresenter(domain::RoboModel* model, QObject* parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model->gamepad();

    connect(d->model, &GamepadModel::capacityChanged,
            this, &GamepadPresenter::batteryLevelChanged);
    connect(d->model, &GamepadModel::chargingChanged,
            this, &GamepadPresenter::chargingChanged);
    connect(d->model, &GamepadModel::connectedChanged,
            this, &GamepadPresenter::connectedChanged);
}

GamepadPresenter::~GamepadPresenter()
{
    delete d;
}

int GamepadPresenter::batteryLevel() const
{
    return d->model->capacity();
}

bool GamepadPresenter::isCharging() const
{
    return d->model->isCharging();
}

bool GamepadPresenter::isConnected() const
{
    return d->model->isConnected();
}

