#include "robo_core.h"

// msgs
#include "influence.h"
#include "joy_axes.h"

#include "pub_sub.h"

#include <QPointF>

#include <limits>

namespace
{
    constexpr float defaultDotsPerDegree = 100;
    constexpr double influenceCoef = 90.0 / 32767;

    enum Axes
    {
        X1 = 0,
        Y1 = 1,
        X2 = 2,
        Y2 = 5
    };

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

    short enginePowerLeft = SHRT_MAX;
    short enginePowerRight = SHRT_MAX;

    Publisher< Influence >* influenceP = nullptr;

    double smooth(double value, double maxInputValue, double maxOutputValue) const;
    void setState(State state);
};

RoboCore::RoboCore():
    ITask(),
    d(new Impl)
{
    d->influenceP = PubSub::instance()->advertise< Influence >("core/influence");

    PubSub::instance()->subscribe("tracker/status", &RoboCore::onTrackerStatusChanged, this);
    PubSub::instance()->subscribe("tracker/deviation", &RoboCore::onTrackerDeviation, this);
    PubSub::instance()->subscribe("camera/dotsPerDegree", &RoboCore::onDotsPerDegreeChanged, this);
    PubSub::instance()->subscribe("core/enginePower", &RoboCore::onEnginePowerChanged, this);
    PubSub::instance()->subscribe("joy/axes", &RoboCore::onJoyEvent, this);
    PubSub::instance()->subscribe("core/shot", &RoboCore::onFireStatusChanged, this);

    d->setState(d->state);
}

RoboCore::~RoboCore()
{
    delete d;
}

void RoboCore::onJoyEvent(const JoyAxes& joy)
{
    if (d->state == State::Search)
    {
        short x = d->smooth(joy.axes[Axes::X2], SHRT_MAX, SHRT_MAX);
        short y = d->smooth(joy.axes[Axes::Y2], SHRT_MAX, SHRT_MAX);
        d->influence.gunV = d->influence.cameraV = -y; // Y axis is inverted. Up is negative, down is positive
        d->influence.towerH = x;
    }

    const float speed = -joy.axes[Axes::Y1]; // Y axis is inverted. Up is negative, down is positive
    const float turnSpeed = joy.axes[Axes::X1];

    d->influence.leftEngine = d->smooth(
                ::bound< float >(-SHRT_MAX, speed - turnSpeed, SHRT_MAX), SHRT_MAX, d->enginePowerLeft);
    d->influence.rightEngine = d->smooth(
                ::bound< float >(-SHRT_MAX, speed + turnSpeed, SHRT_MAX), SHRT_MAX, d->enginePowerRight);
    d->influenceP->publish(d->influence);

//    qDebug() << Q_FUNC_INFO << d->influence.leftEngine << d->influence.leftEngine << d->influence.towerH << joy.axes;

    if (d->state == State::Track)
    {
        d->influence.gunV = d->influence.cameraV = 0;
    }
}

void RoboCore::onTrackerDeviation(const QPointF& deviation)
{
    if (d->state != State::Track) return;

    qDebug() << Q_FUNC_INFO << deviation;
    d->influence.towerH = (deviation.x() / d->dotsPerDegree.x()) / ::influenceCoef; // TODO PID
    d->influence.gunV = d->influence.cameraV =
            (deviation.y() / d->dotsPerDegree.y()) / ::influenceCoef;
    d->influenceP->publish(d->influence);
}

void RoboCore::onEnginePowerChanged(const QPoint& enginePower)
{
    qDebug() << Q_FUNC_INFO << enginePower;
    d->enginePowerLeft = enginePower.x() * SHRT_MAX / 100;
    d->enginePowerRight = enginePower.y() * SHRT_MAX / 100;
}

void RoboCore::onTrackerStatusChanged(const bool& status)
{
    d->setState(status ? State::Track : State::Search);
}

void RoboCore::onFireStatusChanged(const bool& status)
{
    qDebug() << Q_FUNC_INFO << status;
    d->influence.shot = status;
}

void RoboCore::onDotsPerDegreeChanged(const QPointF& dpd)
{
    qDebug() << Q_FUNC_INFO << dpd;
    d->dotsPerDegree = dpd;
}

//------------------------------------------------------------------------------------
double RoboCore::Impl::smooth(double value, double maxInputValue, double maxOutputValue) const
{
    return pow((value / maxInputValue), 3) * maxOutputValue;
}

void RoboCore::Impl::setState(State state)
{
    enum AngleType {Velocity = 0, Position = 1};
    switch (state)
    {
    case State::Search:
        influence.angleType = AngleType::Velocity;
        break;
    case State::Track:
        influence.angleType = AngleType::Position;
        break;
    default:
        break;
    }
}
