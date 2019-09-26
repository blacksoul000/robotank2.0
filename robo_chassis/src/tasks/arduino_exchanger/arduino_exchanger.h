#ifndef ARDUINO_EXCHANGER_H
#define ARDUINO_EXCHANGER_H

#include "i_task.h"

struct Empty;
struct JoyAxes;

class ArduinoExchanger : public ITask
{
public:
    ArduinoExchanger();
    ~ArduinoExchanger();

    void execute() override;

    void onTrackerStatusChanged(const bool& status);
    void onJoyButtonsEvent(const quint16& buttons);
    void onTrackerDeviation(const QPointF& deviation);
    void onNewData(const QByteArray& data);
    void onPowerDown(const Empty&);
    void onGunCalibrate(const Empty&);
    void onGyroCalibrate(const Empty&);
    void onJoyEvent(const JoyAxes& axes);

private:
    class Impl;
    Impl* d;
};

#endif //ARDUINO_EXCHANGER_H
