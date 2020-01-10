#include "presenter_factory.h"
#include "robo_model.h"

#include "track_presenter.h"
#include "settings_presenter.h"
#include "status_presenter.h"
#include "bluetooth_presenter.h"
#include "video_presenter.h"
#include "gamepad_presenter.h"
#include "mavlink_presenter.h"

#include <QtQml>

using presentation::PresenterFactory;
using presentation::SettingsPresenter;
using presentation::StatusPresenter;
using presentation::BluetoothPresenter;
using presentation::VideoPresenter;
using presentation::GamepadPresenter;
using presentation::MavlinkPresenter;

class PresenterFactory::Impl
{
public:
    domain::RoboModel* model;
};

PresenterFactory::PresenterFactory(domain::RoboModel* model, QObject* parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;
}

PresenterFactory::~PresenterFactory()
{
    delete d;
}

QObject* PresenterFactory::trackPresenter()
{
    return new TrackPresenter(d->model, this);
}

QObject* PresenterFactory::settingsPresenter()
{
    return new SettingsPresenter(d->model, this);
}

QObject* PresenterFactory::statusPresenter()
{
    return new StatusPresenter(d->model, this);
}

QObject* PresenterFactory::bluetoothPresenter()
{
    return new BluetoothPresenter(d->model, this);
}

QObject* PresenterFactory::videoPresenter()
{
    return new VideoPresenter(d->model, this);
}

QObject* PresenterFactory::gamepadPresenter()
{
    return new GamepadPresenter(d->model, this);
}

QObject* PresenterFactory::mavlinkPresenter()
{
    return new MavlinkPresenter(d->model, this);
}
