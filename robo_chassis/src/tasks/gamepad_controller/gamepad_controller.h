#ifndef GAMEPAD_CONTROLLER_H
#define GAMEPAD_CONTROLLER_H

#include "i_task.h"

class GamepadController : public ITask
{
    Q_OBJECT
public:
    enum Axes // Bluetooth
    {
        X1 = 0,
        Y1 = 1,
        X2 = 2,
        Y2 = 5,
        DigitalX = 6,
        DigitalY = 7,
    };

    GamepadController();
    ~GamepadController();

    void execute() override;

private slots:
	void onConnectedChanged(bool connected);
    void onCapacityChanged(int capacity);
    void onChargingChanged(bool charging);

private:
    class Impl;
    Impl* d;
};

#endif // GAMEPAD_CONTROLLER_H
