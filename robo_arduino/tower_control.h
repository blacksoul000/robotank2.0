#ifndef TOWER_CONTROL_H
#define TOWER_CONTROL_H

class IImu;
struct Engine;

#include "pid.h"

class TowerControl
{
public:
	TowerControl();
	virtual ~TowerControl();

	void move(short x, short y);
	void updateImuData();
	void updateCurrentData();
	void updatePosition();

	void setStab(bool on);
	void setTracking(bool on);
	void setDeviation(float deviation);
	void setChassisYaw(float yaw);

	float position() const;

	IImu* const imu() const;
	Engine* const engine() const;

private:
	IImu* m_imu = nullptr;
	Engine* m_engine = nullptr;

	bool m_stab = false;
	bool m_tracking = false;
	float m_requiredTowerH = 0.0;
	float m_position = 0.0;

	const double m_Kp = 5.5;
	const double m_Ki = 0.2;
	const double m_Kd = 1.0;
	const double m_dt = 0.1;
	const double m_minInfluence = -40;
	const double m_maxInfluence = 40;

	PID m_pid = PID(m_dt, m_maxInfluence, m_minInfluence, m_Kp, m_Kd, m_Ki);
};

#endif // TOWER_CONTROL_H
