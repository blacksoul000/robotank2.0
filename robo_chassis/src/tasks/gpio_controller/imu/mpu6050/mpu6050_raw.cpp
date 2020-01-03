#include "mpu6050_raw.h"
#include "MPU6050.h"

#include "complementary_filter.h"

#include <QElapsedTimer>

class Mpu6050Raw::Impl
{
public:
	float gyroResolution = 1.0;
	float accelResolution = 1.0;
	float magResolution = 1.0;
	bool isReady = true;

	MPU6050 mpu;
	int16_t ax, ay, az, gx, gy, gz;

	domain::ComplementaryFilter filter;

	QElapsedTimer timer;
};

Mpu6050Raw::Mpu6050Raw() :
    d(new Impl)
{}

Mpu6050Raw::~Mpu6050Raw()
{
    delete d;
}

bool Mpu6050Raw::init()
{
    // initialize device
    d->mpu.initialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    d->mpu.setXGyroOffset(14);
    d->mpu.setYGyroOffset(-53);
    d->mpu.setZGyroOffset(25);
    d->mpu.setXAccelOffset(825);
    d->mpu.setYAccelOffset(304);
    d->mpu.setZAccelOffset(1254);

    d->timer.start();
    return d->mpu.testConnection();
}

bool Mpu6050Raw::isReady() const
{
	return d->isReady;
}

void Mpu6050Raw::readData()
{
	d->mpu.getMotion6(&d->ax, &d->ay, &d->az, &d->gx, &d->gy, &d->gz);
	d->filter.setAccelData(d->accelResolution * d->ax,
							d->accelResolution * d->ay,
							d->accelResolution * d->az);
	d->filter.setGyroData(d->gyroResolution * d->gx,
							d->gyroResolution * d->gy,
							d->gyroResolution * d->gz);
	d->filter.process(d->timer.restart());
}

float Mpu6050Raw::yaw() const
{
	return 0;
}

float Mpu6050Raw::pitch() const
{
	return d->filter.pitch();
}

float Mpu6050Raw::roll() const
{
	return d->filter.roll();
}
