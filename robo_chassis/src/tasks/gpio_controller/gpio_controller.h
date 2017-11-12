#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include "i_task.h"

struct JoyButtons;
struct PointF3D;
struct Influence;
struct Empty;

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
    void onGunCalibrate(const Empty&);
    void onCameraCalibrate(const Empty&);
    void onGyroCalibrate(const Empty&);
    void onChassisGyro(const PointF3D& ypr);
    void onArduinoStatusChanged(const bool& online);
    static void servoTickProxy(void* data);
    void servoTick();

private:
    class Impl;
    Impl* d;
};

#endif //GPIO_CONTROLLER_H
