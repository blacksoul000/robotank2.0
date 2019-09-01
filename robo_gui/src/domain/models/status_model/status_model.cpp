#include "status_model.h"

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

void StatusModel::setChassisImuOnline(bool online)
{
    if (online != d->chassisImuOnline)

	d->chassisImuOnline = online;
    emit chassisImuStatusChanged();
}

bool StatusModel::chassisImuOnline() const
{
	return d->chassisImuOnline;
}

void StatusModel::setChassisImuReady(bool ready)
{
    if (ready != d->chassisImuReady)

	d->chassisImuReady = ready;
    emit chassisImuStatusChanged();
}

bool StatusModel::chassisImuReady() const
{
	return d->chassisImuReady;
}

void StatusModel::setTowerImuOnline(bool online)
{
    if (online != d->towerImuOnline)

	d->towerImuOnline = online;
    emit towerImuStatusChanged();
}

bool StatusModel::towerImuOnline() const
{
	return d->towerImuOnline;
}

void StatusModel::setTowerImuReady(bool ready)
{
    if (ready != d->towerImuReady)

	d->towerImuReady = ready;
    emit towerImuStatusChanged();
}

bool StatusModel::towerImuReady() const
{
	return d->towerImuReady;
}

void StatusModel::setButtons(quint16 buttons)
{
    if (buttons != d->buttons)

    d->buttons = buttons;
    emit buttonsChanged(buttons);
}

quint16 StatusModel::buttons() const
{
    return d->buttons;
}
