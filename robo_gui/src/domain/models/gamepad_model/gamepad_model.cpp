#include "gamepad.h"

//Qt
#include <QMutex>
#include <QMutexLocker>
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
    QMutex mutex;
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
	connect(d->gamepad.data(), &Gamepad::connectedChanged,
			this, &GamepadModel::joyChanged);
	this->startTimer(::checkInterval);
}

GamepadModel::~GamepadModel()
{
	d->gamepad.reset();
    delete d;
}

void GamepadModel::timerEvent(QTimerEvent* event)
{
    QMutexLocker locker(&d->mutex);
	Q_UNUSED(event)

	d->gamepad->execute();
	if (d->gamepad->isConnected()) emit joyChanged();
}

int GamepadModel::capacity() const
{
    QMutexLocker locker(&d->mutex);
	return d->gamepad->isConnected() ? d->gamepad->capacity() : d->capacity;
}

bool GamepadModel::isCharging() const
{
    QMutexLocker locker(&d->mutex);
	return d->gamepad->isConnected() ? d->gamepad->isCharging() : d->isCharging;
}

bool GamepadModel::isConnected() const
{
    QMutexLocker locker(&d->mutex);
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
    QMutexLocker locker(&d->mutex);
    if (d->isCharging == charging) return;

    d->isCharging = charging;
    locker.unlock();

    emit chargingChanged(charging);
}

void GamepadModel::setCapacity(int capacity)
{
    QMutexLocker locker(&d->mutex);
    if (d->capacity == capacity) return;

    d->capacity = capacity;
    locker.unlock();

    emit capacityChanged(capacity);
}

void GamepadModel::setConnected(bool connected)
{
    QMutexLocker locker(&d->mutex);
    if (connected != d->isConnected)

    d->isConnected = connected;
    locker.unlock();

    emit connectedChanged(connected);
}
