#include "status_model.h"

#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

using domain::StatusModel;

class StatusModel::Impl
{
public:
    int batteryLevel = 0;
    bool isCharging = false;
    int robotBatteryLevel = 0;
    qreal gunPositionH = 0;
    qreal gunPositionV = 0;
    qreal yaw = 0;
    qreal pitch = 0;
    qreal roll = 0;
    bool arduinoStatus = false;
    bool chassisStatus = false;

    bool headlightStatus = false;
    bool pointerStatus = false;

    bool chassisImuOnline = false;
    bool chassisImuReady = false;
    bool towerImuOnline = false;
    bool towerImuReady = false;

    quint16 buttons = 0;

    QMutex mutex;
};

StatusModel::StatusModel(QObject* parent) :
    QObject(parent),
    d(new Impl)
{
}

StatusModel::~StatusModel()
{
    delete d;
}

void StatusModel::setBatteryLevel(int level)
{
    QMutexLocker locker(&d->mutex);
    if (d->batteryLevel == level) return;

    d->batteryLevel = level;
    emit batteryLevelChanged(level);
}

int StatusModel::batteryLevel() const
{
    QMutexLocker locker(&d->mutex);
    return d->batteryLevel;
}

void StatusModel::setCharging(bool charging)
{
    QMutexLocker locker(&d->mutex);
    if (d->isCharging == charging) return;

    d->isCharging = charging;
    emit isChargingChanged(charging);
}

bool StatusModel::isCharging() const
{
    QMutexLocker locker(&d->mutex);
    return d->isCharging;
}

void StatusModel::setRobotBatteryLevel(int level)
{
    QMutexLocker locker(&d->mutex);
    if (d->robotBatteryLevel == level) return;

    d->robotBatteryLevel = level;
    emit robotBatteryLevelChanged(level);
}

int StatusModel::robotBatteryLevel() const
{
    QMutexLocker locker(&d->mutex);
    return d->robotBatteryLevel;
}

void StatusModel::setGunPositionH(qreal position)
{
    QMutexLocker locker(&d->mutex);
    if (qFuzzyCompare(d->gunPositionH, position)) return;

    d->gunPositionH = position;
    emit gunPositionHChanged(position);
}

qreal StatusModel::gunPositionH() const
{
    QMutexLocker locker(&d->mutex);
    return d->gunPositionH;
}

void StatusModel::setGunPositionV(qreal position)
{
    QMutexLocker locker(&d->mutex);
    if (qFuzzyCompare(d->gunPositionV, position)) return;

    d->gunPositionV = position;
    emit gunPositionVChanged(position);
}

qreal StatusModel::gunPositionV() const
{
    QMutexLocker locker(&d->mutex);
    return d->gunPositionV;
}

void StatusModel::setYaw(qreal yaw)
{
    QMutexLocker locker(&d->mutex);
    if (qFuzzyCompare(d->yaw, yaw)) return;

    d->yaw = yaw;
    emit yawChanged(yaw);
}

qreal StatusModel::yaw() const
{
    QMutexLocker locker(&d->mutex);
    return d->yaw;
}

void StatusModel::setPitch(qreal pitch)
{
    QMutexLocker locker(&d->mutex);
    if (qFuzzyCompare(d->pitch, pitch)) return;

    d->pitch = pitch;
    emit pitchChanged(pitch);
}

qreal StatusModel::pitch() const
{
    QMutexLocker locker(&d->mutex);
    return d->pitch;
}

void StatusModel::setRoll(qreal roll)
{
    QMutexLocker locker(&d->mutex);
    if (qFuzzyCompare(d->roll, roll)) return;

    d->roll = roll;
    emit rollChanged(roll);
}

qreal StatusModel::roll() const
{
    QMutexLocker locker(&d->mutex);
    return d->roll;
}

void StatusModel::setArduinoStatus(bool status)
{
    QMutexLocker locker(&d->mutex);
    if (status != d->arduinoStatus)

    d->arduinoStatus = status;
    emit arduinoStatusChanged(status);
}

bool StatusModel::arduinoStatus() const
{
    QMutexLocker locker(&d->mutex);
    return d->arduinoStatus;
}

void StatusModel::setChassisStatus(bool status)
{
    QMutexLocker locker(&d->mutex);
    if (status != d->chassisStatus)

    d->chassisStatus = status;
    emit chassisStatusChanged(status);
}

bool StatusModel::chassisStatus() const
{
    QMutexLocker locker(&d->mutex);
    return d->chassisStatus;
}

void StatusModel::setHeadlightStatus(bool on)
{
    QMutexLocker locker(&d->mutex);
    if (on != d->headlightStatus)

	d->headlightStatus = on;
    emit headlightStatusChanged(on);
}

bool StatusModel::headlightStatus() const
{
    QMutexLocker locker(&d->mutex);
	return d->headlightStatus;
}

void StatusModel::setPointerStatus(bool on)
{
    QMutexLocker locker(&d->mutex);
    if (on != d->pointerStatus)

	d->pointerStatus = on;
    emit pointerStatusChanged(on);
}

bool StatusModel::pointerStatus() const
{
    QMutexLocker locker(&d->mutex);
	return d->pointerStatus;
}

void StatusModel::setChassisImuOnline(bool online)
{
    QMutexLocker locker(&d->mutex);
    if (online != d->chassisImuOnline)

	d->chassisImuOnline = online;
    locker.unlock();

    emit chassisImuStatusChanged();
}

bool StatusModel::chassisImuOnline() const
{
    QMutexLocker locker(&d->mutex);
	return d->chassisImuOnline;
}

void StatusModel::setChassisImuReady(bool ready)
{
    QMutexLocker locker(&d->mutex);
    if (ready != d->chassisImuReady)

	d->chassisImuReady = ready;
    locker.unlock();

    emit chassisImuStatusChanged();
}

bool StatusModel::chassisImuReady() const
{
    QMutexLocker locker(&d->mutex);
	return d->chassisImuReady;
}

void StatusModel::setTowerImuOnline(bool online)
{
    QMutexLocker locker(&d->mutex);
    if (online != d->towerImuOnline)

	d->towerImuOnline = online;
    locker.unlock();

    emit towerImuStatusChanged();
}

bool StatusModel::towerImuOnline() const
{
    QMutexLocker locker(&d->mutex);
	return d->towerImuOnline;
}

void StatusModel::setTowerImuReady(bool ready)
{
    QMutexLocker locker(&d->mutex);
    if (ready != d->towerImuReady)

	d->towerImuReady = ready;
    locker.unlock();

    emit towerImuStatusChanged();
}

bool StatusModel::towerImuReady() const
{
    QMutexLocker locker(&d->mutex);
	return d->towerImuReady;
}

void StatusModel::setButtons(quint16 buttons)
{
    QMutexLocker locker(&d->mutex);
    if (buttons != d->buttons)

    d->buttons = buttons;
    locker.unlock();

    emit buttonsChanged(buttons);
}

quint16 StatusModel::buttons() const
{
    QMutexLocker locker(&d->mutex);
    return d->buttons;
}
