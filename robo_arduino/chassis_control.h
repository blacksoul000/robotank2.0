#ifndef CHASSIS_CONTROL_H
#define CHASSIS_CONTROL_H

class IImu;
struct Engine;

class ChassisControl
{
public:
	ChassisControl();
	virtual ~ChassisControl();

	void move(short x, short y);
	void updateImuData();
	void updateCurrentData();

	IImu* const imu() const;
	Engine* const left() const;
	Engine* const right() const;

private:
	IImu* m_imu = nullptr;
	Engine* m_left = nullptr;
	Engine* m_right = nullptr;
};

#endif // CHASSIS_CONTROL_H
