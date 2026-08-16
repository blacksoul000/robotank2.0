#include "robo_core.h"
#include "pid.h"

// msgs
#include "influence.h"
#include "joy_axes.h"

#include "pub_sub.h"

#include <QPointF>

#include <limits>
#include <cmath>

namespace
{
    constexpr float defaultDotsPerDegree = 100;
    constexpr double influenceCoef = 90.0 / 32767;

    // constexpr double turnCoef = 13.6 / 2 / 0.5;  // tracksSeparation / 2 / steeringEfficiency
    constexpr double turnCoef = 1.0;  // tracksSeparation / 2 / steeringEfficiency

    // towerH pid
    const double Kp = 5.5;
    const double Ki = 0.2;
    const double Kd = 1.0;
    const double dt = 0.1;
    const double minInfluence = -40;
    const double maxInfluence = 40;

    template < class T >
    inline T bound(T minValue, T value, T maxValue)
    {
        return std::min(std::max(value, minValue), maxValue);
    }
}

class RoboCore::Impl
{
public:
    State state = State::Search;
    Influence influence;
    QPointF dotsPerDegree;
    QPointF gunPosition;
    PID pid = PID(::dt, ::maxInfluence, ::minInfluence, ::Kp, ::Kd, ::Ki);

    double enginePowerLeft = 1.0;
    double enginePowerRight = 1.0;
    bool hasNewData = false;
    double requiredTowerH = 0;

    Publisher< Influence >* influenceP = nullptr;
    Publisher< double >* deviationVP = nullptr;

    double smooth(double value, double maxInputValue, double maxOutputValue) const;
};

RoboCore::RoboCore():
    ITask(),
    d(new Impl)
{
    d->influenceP = PubSub::instance()->advertise< Influence >("core/influence");
    d->deviationVP = PubSub::instance()->advertise< double >("core/deviationV");

    PubSub::instance()->subscribe("camera/dotsPerDegree", &RoboCore::onDotsPerDegreeChanged, this);
    PubSub::instance()->subscribe("core/enginePower", &RoboCore::onEnginePowerChanged, this);
    PubSub::instance()->subscribe("joy/axes", &RoboCore::onJoyEvent, this);
    PubSub::instance()->subscribe("gun/position", &RoboCore::onGunPosition, this);

    d->state = State::Search;
}

RoboCore::~RoboCore()
{
    delete d->influenceP;
    delete d->deviationVP;
    delete d;
}

void RoboCore::execute()
{
    d->influenceP->publish(d->influence);
}

void RoboCore::onJoyEvent(const JoyAxes& joy)
{
    d->influence.gunV = joy.y2;
    d->influence.towerH = joy.x2;

    const int speed = d->smooth(joy.y1, SHRT_MAX, SHRT_MAX);
    int turn = d->smooth(joy.x1, SHRT_MAX, SHRT_MAX);
    if (speed < 0) turn *= -1;

    d->influence.leftEngine = ::bound< int >(SHRT_MIN,
            (speed + turn * ::turnCoef) * d->enginePowerLeft, SHRT_MAX);
    d->influence.rightEngine = ::bound< int >(SHRT_MIN,
            (speed - turn * ::turnCoef) * d->enginePowerRight, SHRT_MAX);
}

void RoboCore::onEnginePowerChanged(const QPoint& enginePower)
{
    d->enginePowerLeft = enginePower.x() / 100.0;
    d->enginePowerRight = enginePower.y() / 100.0;
}

void RoboCore::onGunPosition(const QPointF& position)
{
    d->gunPosition = position;
}

void RoboCore::onDotsPerDegreeChanged(const QPointF& dpd)
{
    d->dotsPerDegree = dpd;
}

void RoboCore::onCommandReceived(const QJsonObject& command)
{
    // Обработка команд от телефона через TCP
    QString type = command["type"].toString();
    
    if (type == "COMMAND") {
        QJsonObject leftObj = command["left"].toObject();
        QJsonObject rightObj = command["right"].toObject();
        
        double leftX = leftObj["x"].toDouble(0.0);
        double leftY = leftObj["y"].toDouble(0.0);
        double rightX = rightObj["x"].toDouble(0.0);
        double rightY = rightObj["y"].toDouble(0.0);
        
        // Преобразуем данные джойстиков в формат JoyAxes
        JoyAxes joy;
        joy.x1 = static_cast<short>(leftX * SHRT_MAX);
        joy.y1 = static_cast<short>(leftY * SHRT_MAX);
        joy.x2 = static_cast<short>(rightX * SHRT_MAX);
        joy.y2 = static_cast<short>(rightY * SHRT_MAX);
        
        onJoyEvent(joy);
    }
}

//------------------------------------------------------------------------------------
double RoboCore::Impl::smooth(double value, double maxInputValue, double maxOutputValue) const
{
    return pow((value / maxInputValue), 3) * maxOutputValue;
}
