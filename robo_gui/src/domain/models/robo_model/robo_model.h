#ifndef ROBO_MODEL_H
#define ROBO_MODEL_H

namespace domain
{
    class StatusModel;
    class TrackModel;
    class SettingsModel;
    class BluetoothModel;
    class GamepadModel;
    class MavlinkExchanger;

    class RoboModel
    {
    public:
        RoboModel();
        ~RoboModel();

        StatusModel* status() const;
        TrackModel* track() const;
        SettingsModel* settings() const;
        BluetoothModel* bluetooth() const;
        GamepadModel* gamepad() const;
        MavlinkExchanger* mavlink() const;

    private:
        StatusModel* m_status = nullptr;
        TrackModel* m_track = nullptr;
        SettingsModel* m_settings = nullptr;
        BluetoothModel* m_bluetooth = nullptr;
        GamepadModel* m_gamepad = nullptr;
        MavlinkExchanger* m_mavlink = nullptr;
    };
}

#endif //ROBO_MODEL_H
