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

    constexpr double turnCoef = 13.6 / 2 / 0.5;  // tracksSeparation / 2 / steeringEfficiency

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

    short enginePowerLeft = SHRT_MAX;
    short enginePowerRight = SHRT_MAX;
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

    PubSub::instance()->subscribe("tracker/status", &RoboCore::onTrackerStatusChanged, this);
    PubSub::instance()->subscribe("tracker/deviation", &RoboCore::onTrackerDeviation, this);
    PubSub::instance()->subscribe("camera/dotsPerDegree", &RoboCore::onDotsPerDegreeChanged, this);
    PubSub::instance()->subscribe("core/enginePower", &RoboCore::onEnginePowerChanged, this);
    PubSub::instance()->subscribe("joy/axes", &RoboCore::onJoyEvent, this);
    PubSub::instance()->subscribe("gun/position", &RoboCore::onGunPosition, this);

    this->onTrackerStatusChanged(false);
}

RoboCore::~RoboCore()
{
    delete d->influenceP;
    delete d->deviationVP;
    delete d;
}

void RoboCore::execute()
{
    if (d->state == State::Track)
    {
        if (std::fabs(d->requiredTowerH - d->gunPosition.x()) > 0.1)
        {
            d->influence.towerH = d->pid.calculate(d->requiredTowerH, d->gunPosition.x()) / ::influenceCoef;
//            qDebug() << Q_FUNC_INFO << d->requiredTowerH << d->gunPosition.x() << d->influence.towerH << (d->influence.towerH * ::influenceCoef);
        }
    }

	d->influenceP->publish(d->influence);
//        qDebug() << Q_FUNC_INFO << d->influence.leftEngine << d->influence.rightEngine << d->influence.gunV << d->influence.towerH;
}

void RoboCore::onJoyEvent(const JoyAxes& joy)
{
    if (d->state == State::Search)
    {
//		qDebug() << Q_FUNC_INFO << __LINE__ << joy.axes;
		short x = d->smooth(joy.x2, SHRT_MAX, SHRT_MAX);
		short y = d->smooth(joy.y2, SHRT_MAX, SHRT_MAX);

		d->influence.gunV = y;
		d->influence.towerH = x;
//		qDebug() << Q_FUNC_INFO << d->influence.gunV << d->influence.towerH << joy.axes;
    }

	const int speed = joy.y1;
	int turn = joy.x1;
	if (speed < 0) turn *= -1;

	d->influence.leftEngine = ::bound< int >(SHRT_MIN,
			d->smooth(speed + turn * ::turnCoef, SHRT_MAX, d->enginePowerLeft), SHRT_MAX);
	d->influence.rightEngine = ::bound< int >(SHRT_MIN,
			d->smooth(speed - turn * ::turnCoef, SHRT_MAX, d->enginePowerRight), SHRT_MAX);
//	qDebug() << Q_FUNC_INFO << __LINE__ << joy.x1 << joy.y1 << joy.x2 << joy.y2
//			<< d->influence.leftEngine << d->influence.rightEngine;
}

void RoboCore::onTrackerDeviation(const QPointF& deviation)
{
    if (d->state != State::Track) return;

    d->requiredTowerH = d->gunPosition.x() + (deviation.x() / d->dotsPerDegree.x());

    double gunV = d->gunPosition.y() - qMin((deviation.y() / d->dotsPerDegree.y()), 1.0);
//    qDebug() << Q_FUNC_INFO << d->gunPosition.y() << deviation.y() << d->dotsPerDegree.y() << gunV << (deviation.y() / d->dotsPerDegree.y());
    d->deviationVP->publish(gunV);
}

void RoboCore::onEnginePowerChanged(const QPoint& enginePower)
{
    d->enginePowerLeft = enginePower.x() * SHRT_MAX / 100;
    d->enginePowerRight = enginePower.y() * SHRT_MAX / 100;
}

void RoboCore::onGunPosition(const QPointF& position)
{
    d->gunPosition = position;
}

void RoboCore::onTrackerStatusChanged(const bool& status)
{
    if (status)
    {
        d->requiredTowerH = d->gunPosition.x();
        d->state = State::Track;
    }
    else
    {
        d->state = State::Search;
    }
    d->influence.gunV = 0;
    d->influence.towerH = 0;
    d->hasNewData = true;
}

void RoboCore::onDotsPerDegreeChanged(const QPointF& dpd)
{
    d->dotsPerDegree = dpd;
}

//------------------------------------------------------------------------------------
double RoboCore::Impl::smooth(double value, double maxInputValue, double maxOutputValue) const
{
    return pow((value / maxInputValue), 3) * maxOutputValue;
}
