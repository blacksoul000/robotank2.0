#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include "i_task.h"

struct Influence;
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
    void readGyroData();
    void onJoyEvent(const quint16& joy);
    void onInfluence(const Influence& influence);
    void onDeviation(const double& value);
    void onGunCalibrate(const Empty&);

    static void servoTickProxy(void* data);
    void servoTick();

private:
    class Impl;
    Impl* d;
};

#endif //GPIO_CONTROLLER_H
