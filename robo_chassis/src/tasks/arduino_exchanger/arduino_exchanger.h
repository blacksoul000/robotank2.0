#ifndef ARDUINO_EXCHANGER_H
#define ARDUINO_EXCHANGER_H

#include "i_task.h"

struct Influence;

class ArduinoExchanger : public ITask
{
public:
    ArduinoExchanger();
    ~ArduinoExchanger();

    void start() override;
    void execute() override;

    void onInfluence(const Influence& influence);
    void onJoyEvent(const quint16& joy);

private:
    class Impl;
    Impl* d;
};

#endif //ARDUINO_EXCHANGER_H
