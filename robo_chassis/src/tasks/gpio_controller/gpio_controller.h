#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include "i_task.h"

struct JoyButtons;

class GpioController : public ITask
{
public:
    GpioController();
    ~GpioController();

    void execute() override;

    void onJoyEvent(const quint16& joy);
    void onArduinoStatusChanged(const bool& online);

private:
    class Impl;
    Impl* d;
};

#endif //GPIO_CONTROLLER_H
