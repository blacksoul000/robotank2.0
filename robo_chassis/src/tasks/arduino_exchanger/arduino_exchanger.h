#ifndef ARDUINO_EXCHANGER_H
#define ARDUINO_EXCHANGER_H

#include "i_task.h"

#include <QString>

struct Influence;
struct Empty;

class ArduinoExchanger : public ITask
{
public:
    ArduinoExchanger();
    ~ArduinoExchanger();

    void start() override;
    void execute() override;

    void onInfluence(const Influence& influence);
    void onGunCalibrate(const Empty&);
    void onCameraCalibrate(const Empty&);
    void onGyroCalibrate(const Empty &);

private:
    class Impl;
    Impl* d;
};

#endif //ARDUINO_EXCHANGER_H
