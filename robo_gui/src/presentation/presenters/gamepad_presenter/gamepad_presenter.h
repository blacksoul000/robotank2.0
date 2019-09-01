#ifndef GAMEPAD_PRESENTER_H
#define GAMEPAD_PRESENTER_H

#include <QObject>

namespace domain
{
    class RoboModel;
}

namespace presentation
{
    class GamepadPresenter : public QObject
    {
        Q_OBJECT
    public:
        Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY batteryLevelChanged)
        Q_PROPERTY(bool charging READ isCharging NOTIFY chargingChanged)
        Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

        GamepadPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~GamepadPresenter() override;

        int batteryLevel() const;
        bool isCharging() const;
        bool isConnected() const;

    signals:
        void batteryLevelChanged(int level);
        void chargingChanged(bool charging);
        void connectedChanged(bool online);

    private:
        class Impl;
        Impl* d;
    };
}

#endif //GAMEPAD_PRESENTER_H
