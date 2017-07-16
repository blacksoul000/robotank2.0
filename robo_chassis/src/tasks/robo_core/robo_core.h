#ifndef ROBO_CORE_H
#define ROBO_CORE_H

#include "i_task.h"

struct JoyAxes;

class RoboCore : public ITask
{
public:
    enum class State
    {
        Search,
        Track
    };

    RoboCore();
    ~RoboCore();

    void execute() override;

    void onTrackerStatusChanged(const bool& status);
    void onTrackerDeviation(const QPointF& deviation);
    void onDotsPerDegreeChanged(const QPointF& dpd);
    void onEnginePowerChanged(const QPoint& enginePower);
    void onJoyEvent(const JoyAxes& axes);
    void onFireStatusChanged(const bool &status);

private:
    class Impl;
    Impl* d;
};

#endif //ROBO_CORE_H
