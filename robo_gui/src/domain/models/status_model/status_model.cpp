#include "status_model.h"

using domain::StatusModel;

class StatusModel::Impl
{
public:
    int batteryLevel = 0;
    bool isCharging = false;
    int gamepadBatteryLevel = 0;
    int robotBatteryLevel = 0;
    int gunPositionH = 0;
    int gunPositionV = 0;
    int cameraPositionV = 0;
    qreal yaw = 0;
    qreal pitch = 0;
    qreal roll = 0;
    bool arduinoStatus = false;
    bool joyStatus = false;
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

void StatusModel::setGunPositionH(int position)
{
    if (d->gunPositionH == position) return;

    d->gunPositionH = position;
    emit gunPositionHChanged(position);
}

int StatusModel::gunPositionH() const
{
    return d->gunPositionH;
}

void StatusModel::setGunPositionV(int position)
{
    if (d->gunPositionV == position) return;

    d->gunPositionV = position;
    emit gunPositionVChanged(position);
}

int StatusModel::gunPositionV() const
{
    return d->gunPositionV;
}

void StatusModel::setCameraPositionV(int position)
{
    if (d->cameraPositionV == position) return;

    d->cameraPositionV = position;
    emit cameraPositionVChanged(position);
}

int StatusModel::cameraPositionV() const
{
    return d->cameraPositionV;
}

void StatusModel::setYaw(qreal yaw)
{
    d->yaw = yaw;
    emit yawChanged(yaw);
}

qreal StatusModel::yaw() const
{
    return d->yaw;
}

void StatusModel::setPitch(qreal pitch)
{
    d->pitch = pitch;
    emit pitchChanged(pitch);
}

qreal StatusModel::pitch() const
{
    return d->pitch;
}

void StatusModel::setRoll(qreal roll)
{
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

void StatusModel::setJoyStatus(bool status)
{
    if (status != d->joyStatus)

    d->joyStatus = status;
    emit joyStatusChanged(status);
}

bool StatusModel::joyStatus() const
{
    return d->joyStatus;
}
