#include "robo_model.h"
#include "track_model.h"
#include "settings_model.h"
#include "status_model.h"
#include "bluetooth_model.h"
#include "gamepad_model.h"
#include "mavlink_exchanger.h"

using domain::RoboModel;
using domain::TrackModel;
using domain::SettingsModel;
using domain::StatusModel;
using domain::BluetoothModel;
using domain::GamepadModel;
using domain::MavlinkExchanger;

RoboModel::RoboModel(QObject* parent) :
    QObject(parent)
{
    m_track = new TrackModel(this);
    m_settings = new SettingsModel(this);
    m_status = new StatusModel(this);
    m_bluetooth = new BluetoothModel(this);
    m_gamepad = new GamepadModel(this);
    m_mavlink= new MavlinkExchanger(this);
}

RoboModel::~RoboModel()
{
    delete m_status;
    delete m_track;
    delete m_settings;
    delete m_bluetooth;
    delete m_gamepad;
    delete m_mavlink;
}

StatusModel* RoboModel::status() const
{
    return m_status;
}

TrackModel* RoboModel::track() const
{
    return m_track;
}

SettingsModel* RoboModel::settings() const
{
    return m_settings;
}

BluetoothModel* RoboModel::bluetooth() const
{
    return m_bluetooth;
}

GamepadModel* RoboModel::gamepad() const
{
    return m_gamepad;
}

MavlinkExchanger* RoboModel::mavlink() const
{
    return m_mavlink;
}
