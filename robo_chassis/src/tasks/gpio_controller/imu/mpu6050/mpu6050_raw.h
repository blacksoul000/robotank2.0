#ifndef MPU6050_RAW_H
#define MPU6050_RAW_H

#include "i_imu.h"

#include "stdint.h"

class Mpu6050Raw : public IImu
{
public:
    Mpu6050Raw();
    ~Mpu6050Raw();

    bool init() override;
    bool isReady() const override;
    void readData() override;

    float yaw() const override;
    float pitch() const override;
    float roll() const override;

private:
    class Impl;
    Impl* d;
};

#endif // MPU6050_RAW_H
