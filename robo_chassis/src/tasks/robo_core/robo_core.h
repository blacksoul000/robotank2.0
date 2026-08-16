#ifndef ROBO_CORE_H
#define ROBO_CORE_H

#include "i_task.h"
#include <QJsonObject>

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
    void onGunPosition(const QPointF& position);
    
    // Прием команд от TCP сервера (от телефона)
    void onCommandReceived(const QJsonObject& command);

private:
    class Impl;
    Impl* d;
};

#endif //ROBO_CORE_H
