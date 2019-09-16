#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include "i_task.h"

struct JoyAxes;
struct Empty;

class QPointF;

class GpioController : public ITask
{
public:
    GpioController();
    ~GpioController();

    void start() override;
    void execute() override;

private:
    void onJoyEvent(const JoyAxes& joy);
    void onJoyButtons(const quint16& joy);
    void onTrackerDeviation(const QPointF& deviation);
    void onTrackerStatusChanged(const bool& status);
    void onGunCalibrate(const Empty&);

    static void servoTickProxy(void* data);
    void servoTick();

private:
    class Impl;
    Impl* d;
};

#endif //GPIO_CONTROLLER_H
