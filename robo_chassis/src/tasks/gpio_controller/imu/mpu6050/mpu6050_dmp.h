#ifndef MPU6050_DMP_H
#define MPU6050_DMP_H

#include "i_imu.h"

#include <stdint.h>
#include <cmath>

class Mpu6050Dmp : public IImu
{
public:
	Mpu6050Dmp();
    ~Mpu6050Dmp();

    bool init() override;
    void readData() override;

    float yaw() const override;
    float pitch() const override;
    float roll() const override;

private:
    class Impl;
    Impl* d;
};

#endif // MPU6050_DMP_H
