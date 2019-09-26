#include "tower_control.h"

#include "pinout.h"
#include "common.h"
#include "mpu6050_dmp.h"
#include "engine.h"
#include "i_imu.h"

TowerControl::TowerControl()
{
	m_engine = new Engine{pinTowerCw, pinTowerCcw, pinTowerPwm, pinTowerCs, 0};

  m_engine->init();

  m_imu = new Mpu6050Dmp(0x69, 14, -53, 25, 825, 304, 1254);

  if (!m_imu->init())
  {
    Serial.println("Failed to init 'tower' IMU");
  }
}

TowerControl::~TowerControl()
{
	delete m_imu;
	delete m_engine;
}

IImu* const TowerControl::imu() const
{
	return m_imu;
}

Engine* const TowerControl::engine() const
{
	return m_engine;
}

void TowerControl::move(short x, short)
{
	if (m_tracking) return;

	m_engine->applySpeed(common::smooth(x, 32767, 255));
}

void TowerControl::updateImuData()
{
	m_imu->readData();
}

void TowerControl::updateCurrentData()
{
    m_engine->currentValue = analogRead(pinTowerCs);
}

void TowerControl::updatePosition()
{
	if (m_tracking)
	{
    if (!m_imu->isReady()) return;
    
		if (common::fuzzyCompare(m_requiredTowerH, m_position, 0.1))
			return;

    m_engine->applySpeed(m_pid.calculate(m_requiredTowerH, m_position));
	}
	else if (m_stab)
	{
		// TODO: implement
	}
}

void TowerControl::setStab(bool on)
{
	m_stab = on;
}

void TowerControl::setTracking(bool on)
{
	m_tracking = on;
}

void TowerControl::setDeviation(float deviation)
{
	m_requiredTowerH = m_position + deviation;
}

void TowerControl::setChassisYaw(float yaw)
{
	m_position = m_imu->yaw() - yaw;
}

float TowerControl::position() const
{
	return m_position;
}
