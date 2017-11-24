#ifndef GAMEPAD_CONTROLLER_H
#define GAMEPAD_CONTROLLER_H

#include "i_task.h"

class GamepadController : public ITask
{
    Q_OBJECT
public:
    GamepadController();
    ~GamepadController();

    void execute() override;

private:
    void readData();

private slots:
    void onCapacityChanged();
    void onStatusChanged();

private:
    class Impl;
    Impl* d;
};

#endif // GAMEPAD_CONTROLLER_H
