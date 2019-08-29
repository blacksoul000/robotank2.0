#include "robo_model.h"
#include "track_model.h"
#include "settings_model.h"
#include "status_model.h"
#include "bluetooth_model.h"
#include "gamepad_model.h"

using domain::RoboModel;
using domain::TrackModel;
using domain::SettingsModel;
using domain::StatusModel;
using domain::BluetoothModel;
using domain::GamepadModel;

RoboModel::RoboModel()
{
    m_track = new TrackModel();
    m_settings = new SettingsModel();
    m_status = new StatusModel();
    m_bluetooth = new BluetoothModel();
    m_gamepad = new GamepadModel();
}

RoboModel::~RoboModel()
{
    delete m_status;
    delete m_track;
    delete m_settings;
    delete m_bluetooth;
    delete m_gamepad;
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
