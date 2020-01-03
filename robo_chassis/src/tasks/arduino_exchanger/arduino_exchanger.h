#ifndef ARDUINO_EXCHANGER_H
#define ARDUINO_EXCHANGER_H

#include "i_task.h"

struct Influence;
struct Empty;

class ArduinoExchanger : public ITask
{
public:
    ArduinoExchanger();
    ~ArduinoExchanger();

    void execute() override;

    void onInfluence(const Influence& influence);
    void onJoyEvent(const quint16& buttons);
    void onNewData(const QByteArray& data);
    void onPowerDown(const Empty&);

private:
    class Impl;
    Impl* d;
};

#endif //ARDUINO_EXCHANGER_H
