#include "gamepad.h"

//Qt
#include <QScopedPointer>
#include <QDebug>
#include "gamepad_model.h"

namespace
{
	const int checkInterval = 100;
}  // namespace

using domain::GamepadModel;

class GamepadModel::Impl
{
public:
    QScopedPointer< Gamepad > gamepad;

    bool isCharging = false;
    int capacity = 0;
    bool isConnected = false;
};

GamepadModel::GamepadModel(QObject* parent) :
    QObject(parent),
    d(new Impl)
{
	d->gamepad.reset(new Gamepad("/dev/input/js0", this));

	connect(d->gamepad.data(), &Gamepad::capacityChanged,
			this, &GamepadModel::capacityChanged);
	connect(d->gamepad.data(), &Gamepad::chargingChanged,
			this, &GamepadModel::chargingChanged);

	connect(d->gamepad.data(), &Gamepad::connectedChanged,
			this, &GamepadModel::connectedChanged);
	connect(d->gamepad.data(), &Gamepad::connectedChanged,
			this, &GamepadModel::capacityChanged);
	connect(d->gamepad.data(), &Gamepad::connectedChanged,
			this, &GamepadModel::chargingChanged);
	this->startTimer(::checkInterval);
}

GamepadModel::~GamepadModel()
{
	d->gamepad.reset();
    delete d;
}

void GamepadModel::timerEvent(QTimerEvent* event)
{
	Q_UNUSED(event)

	d->gamepad->execute();
	emit joyChanged();
}

int GamepadModel::capacity() const
{
	return d->gamepad->isConnected() ? d->gamepad->capacity() : d->capacity;
}

bool GamepadModel::isCharging() const
{
	return d->gamepad->isConnected() ? d->gamepad->isCharging() : d->isCharging;
}

bool GamepadModel::isConnected() const
{
	return d->gamepad->isConnected() ? d->gamepad->isConnected() : d->isConnected;
}

QVector< short > GamepadModel::axes() const
{
	return d->gamepad->axes();
}

quint16 GamepadModel::buttons() const
{
	return d->gamepad->buttons();
}

void GamepadModel::setCharging(bool charging)
{
    if (d->isCharging == charging) return;

    d->isCharging = charging;
    emit chargingChanged(charging);
}

void GamepadModel::setCapacity(int capacity)
{
    if (d->capacity == capacity) return;

    d->capacity = capacity;
    emit capacityChanged(capacity);
}

void GamepadModel::setConnected(bool connected)
{
    if (connected != d->isConnected)

    d->isConnected = connected;
    emit connectedChanged(connected);
}
