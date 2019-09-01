#ifndef MPU6050_DMP_H
#define MPU6050_DMP_H

#include "i_imu.h"

#include <stdint.h>

class Mpu6050Dmp : public IImu
{
public:
	Mpu6050Dmp(int8_t deviceId, int8_t xGyro, int8_t yGyro, int8_t zGyro,
			int16_t xAccel, int16_t yAccel, int16_t zAccel);
    ~Mpu6050Dmp();

    bool init() override;
    bool isReady() const override;
    bool isOnline() const override;
    void readData() override;

    float yaw() const override;
    float pitch() const override;
    float roll() const override;

private:
    class Impl;
    Impl* d;
};

#endif // MPU6050_DMP_H
