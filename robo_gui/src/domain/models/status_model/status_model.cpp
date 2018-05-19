#include "status_model.h"

#include <QDebug>

using domain::StatusModel;

class StatusModel::Impl
{
public:
    int batteryLevel = 0;
    bool isCharging = false;
    bool isGamepadCharging = false;
    int gamepadBatteryLevel = 0;
    int robotBatteryLevel = 0;
    qreal gunPositionH = 0;
    qreal gunPositionV = 0;
    qreal cameraPositionV = 0;
    qreal yaw = 0;
    qreal pitch = 0;
    qreal roll = 0;
    bool arduinoStatus = false;
    bool gamepadStatus = false;
    bool chassisStatus = false;

    bool headlightStatus = false;
    bool pointerStatus = false;
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
    if (d->batteryLevel == level) return;

    d->batteryLevel = level;
    emit batteryLevelChanged(level);
}

int StatusModel::batteryLevel() const
{
    return d->batteryLevel;
}

void StatusModel::setCharging(bool charging)
{
    if (d->isCharging == charging) return;

    d->isCharging = charging;
    emit isChargingChanged(charging);
}

bool StatusModel::isCharging() const
{
    return d->isCharging;
}

void StatusModel::setGamepadCharging(bool charging)
{
    if (d->isGamepadCharging == charging) return;

    d->isGamepadCharging = charging;
    emit gamepadChargingChanged(charging);
}

bool StatusModel::gamepadCharging() const
{
	return d->isGamepadCharging;
}

void StatusModel::setGamepadBatteryLevel(int level)
{
    if (d->gamepadBatteryLevel == level) return;

    d->gamepadBatteryLevel = level;
    emit gamepadBatteryLevelChanged(level);
}

int StatusModel::gamepadBatteryLevel() const
{
    return d->gamepadBatteryLevel;
}

void StatusModel::setRobotBatteryLevel(int level)
{
    if (d->robotBatteryLevel == level) return;

    d->robotBatteryLevel = level;
    emit robotBatteryLevelChanged(level);
}

int StatusModel::robotBatteryLevel() const
{
    return d->robotBatteryLevel;
}

void StatusModel::setGunPositionH(qreal position)
{
    if (qFuzzyCompare(d->gunPositionH, position)) return;

    d->gunPositionH = position;
    emit gunPositionHChanged(position);
}

qreal StatusModel::gunPositionH() const
{
    return d->gunPositionH;
}

void StatusModel::setGunPositionV(qreal position)
{
    if (qFuzzyCompare(d->gunPositionV, position)) return;

    d->gunPositionV = position;
    emit gunPositionVChanged(position);
}

qreal StatusModel::gunPositionV() const
{
    return d->gunPositionV;
}

void StatusModel::setCameraPositionV(qreal position)
{
    if (qFuzzyCompare(d->cameraPositionV, position)) return;

    d->cameraPositionV = position;
    emit cameraPositionVChanged(position);
}

qreal StatusModel::cameraPositionV() const
{
    return d->cameraPositionV;
}

void StatusModel::setYaw(qreal yaw)
{
    if (qFuzzyCompare(d->yaw, yaw)) return;

    d->yaw = yaw;
    emit yawChanged(yaw);
}

qreal StatusModel::yaw() const
{
    return d->yaw;
}

void StatusModel::setPitch(qreal pitch)
{
    if (qFuzzyCompare(d->pitch, pitch)) return;

    d->pitch = pitch;
    emit pitchChanged(pitch);
}

qreal StatusModel::pitch() const
{
    return d->pitch;
}

void StatusModel::setRoll(qreal roll)
{
    if (qFuzzyCompare(d->roll, roll)) return;

    d->roll = roll;
    emit rollChanged(roll);
}

qreal StatusModel::roll() const
{
    return d->roll;
}

void StatusModel::setArduinoStatus(bool status)
{
    if (status != d->arduinoStatus)

    d->arduinoStatus = status;
    emit arduinoStatusChanged(status);
}

bool StatusModel::arduinoStatus() const
{
    return d->arduinoStatus;
}

void StatusModel::setGamepadStatus(bool status)
{
    if (status != d->gamepadStatus)

    d->gamepadStatus = status;
    emit gamepadStatusChanged(status);
}

bool StatusModel::gamepadStatus() const
{
    return d->gamepadStatus;
}

void StatusModel::setChassisStatus(bool status)
{
    if (status != d->chassisStatus)

	d->chassisStatus = status;
    emit chassisStatusChanged(status);
}

bool StatusModel::chassisStatus() const
{
    return d->chassisStatus;
}

void StatusModel::setHeadlightStatus(bool on)
{
    if (on != d->headlightStatus)

	d->headlightStatus = on;
    emit headlightStatusChanged(on);
}

bool StatusModel::headlightStatus() const
{
	return d->headlightStatus;
}

void StatusModel::setPointerStatus(bool on)
{
    if (on != d->pointerStatus)

	d->pointerStatus = on;
    emit pointerStatusChanged(on);
}

bool StatusModel::pointerStatus() const
{
	return d->pointerStatus;
}

