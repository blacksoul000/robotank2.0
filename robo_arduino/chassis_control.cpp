#include "chassis_control.h"

#include "pinout.h"
#include "common.h"
#include "mpu6050_dmp.h"
#include "engine.h"
#include "i_imu.h"

namespace
{
	constexpr double turnCoef = 13.6 / 2 / 0.5;  // tracksSeparation / 2 / steeringEfficiency
}

ChassisControl::ChassisControl()
{
  m_left = new Engine{pinLeftCw, pinLeftCcw, pinLeftPwm, pinLeftCs, 0};
  m_right = new Engine{pinRightCw, pinRightCcw, pinRightPwm, pinRightCs, 0};

  m_left->init();
  m_right->init();

  m_imu = new Mpu6050Dmp(0x68, 68, -17, -31, -2364, -983, 987);

  if (!m_imu->init())
  {
    Serial.println("Failed to init 'chassis' IMU");
  }
}

ChassisControl::~ChassisControl()
{
	delete m_imu;
	delete m_left;
	delete m_right;
}

IImu* const ChassisControl::imu() const
{
	return m_imu;
}

Engine* const ChassisControl::left() const
{
	return m_left;
}

Engine* const ChassisControl::right() const
{
	return m_right;
}

void ChassisControl::move(short x, short y)
{
  const int speed = common::smooth(y, 32767, 255);
  int turn = common::smooth(x, 32767, 255) * ::turnCoef;
  if (speed < 0) turn *= -1;

  m_left->applySpeed(common::bound(-255, speed + turn, 255));
  m_right->applySpeed(common::bound(-255, speed - turn, 255));
}

void ChassisControl::updateImuData()
{
	m_imu->readData();
}

void ChassisControl::updateCurrentData()
{
    m_left->currentValue = analogRead(pinLeftCs);
    m_right->currentValue = analogRead(pinRightCs);
}
