#include "mpu6050_dmp.h"

#include "MPU6050_6Axis_MotionApps20.h"

#include <QDebug>

class Mpu6050Dmp::Impl
{
public:
	MPU6050 mpu;

	// MPU control/status vars
	bool dmpReady = false;  // set true if DMP init was successful
	uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
	uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
	uint16_t fifoCount;     // count of all bytes currently in FIFO
	uint8_t fifoBuffer[64]; // FIFO storage buffer

	// orientation/motion vars
	Quaternion q;           // [w, x, y, z]         quaternion container
	VectorInt16 aa;         // [x, y, z]            accel sensor measurements
	VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
	VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
	VectorFloat gravity;    // [x, y, z]            gravity vector
	float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
};

Mpu6050Dmp::Mpu6050Dmp() :
    d(new Impl)
{}

Mpu6050Dmp::~Mpu6050Dmp()
{
    delete d;
}

bool Mpu6050Dmp::init()
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

    if (!d->mpu.testConnection()) return false;

	// load and configure the DMP
	d->devStatus = d->mpu.dmpInitialize();

	// make sure it worked (returns 0 if so)
	if (d->devStatus == 0)
	{
		// turn on the DMP, now that it's ready
		printf("Enabling DMP...\n");
		d->mpu.setDMPEnabled(true);

		// set our DMP Ready flag so the main loop() function knows it's okay to use it
		printf("DMP ready!\n");
		d->dmpReady = true;

		// get expected DMP packet size for later comparison
		d->packetSize = d->mpu.dmpGetFIFOPacketSize();
		return true;
	}
	else
	{
		// ERROR!
		// 1 = initial memory load failed
		// 2 = DMP configuration updates failed
		// (if it's going to break, usually the code will be 1)
		printf("DMP Initialization failed (code %d)\n", d->devStatus);
		return false;
	}
}

void Mpu6050Dmp::readData()
{
    // if programming failed, don't try to do anything
    if (!d->dmpReady) return;

    // get current FIFO count
    d->fifoCount = d->mpu.getFIFOCount();
    if (d->fifoCount == 0) return;

    if (d->fifoCount == 1024 || d->fifoCount % d->packetSize != 0)
    {
        // reset so we can continue cleanly
        d->mpu.resetFIFO();
    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    }
    else
    {
        while (d->fifoCount >= d->packetSize) 
        {
            // read a packet from FIFO
            d->mpu.getFIFOBytes(d->fifoBuffer, d->packetSize);
            d->fifoCount -= d->packetSize;
        }

        // display Euler angles in degrees
        d->mpu.dmpGetQuaternion(&d->q, d->fifoBuffer);
        d->mpu.dmpGetGravity(&d->gravity, &d->q);
        d->mpu.dmpGetYawPitchRoll(d->ypr, &d->q, &d->gravity);
//        printf("ypr1  %7.2f %7.2f %7.2f    ", d->ypr[0] * 180/M_PI, d->ypr[1] * 180/M_PI, d->ypr[2] * 180/M_PI);
        if (d->ypr[0] < 0) d->ypr[0] += 2 * M_PI;

        #ifdef OUTPUT_READABLE_REALACCEL
            // display real acceleration, adjusted to remove gravity
            d->mpu.dmpGetQuaternion(&d->q, d->fifoBuffer);
            d->mpu.dmpGetAccel(&d->aa, d->fifoBuffer);
            d->mpu.dmpGetGravity(&d->gravity, &d->q);
            d->mpu.dmpGetLinearAccel(&d->aaReal, &d->aa, &d->gravity);
            printf("areal %6d %6d %6d    ", d->aaReal.x, d->aaReal.y, d->aaReal.z);
        #endif

        #ifdef OUTPUT_READABLE_WORLDACCEL
            // display initial world-frame acceleration, adjusted to remove gravity
            // and rotated based on known orientation from quaternion
            d->mpu.dmpGetQuaternion(&d->q, d->fifoBuffer);
            d->mpu.dmpGetAccel(&d->aa, d->fifoBuffer);
            d->mpu.dmpGetGravity(&d->gravity, &d->q);
            d->mpu.dmpGetLinearAccelInWorld(&d->aaWorld, &d->aaReal, &d->q);
            printf("aworld %6d %6d %6d    ", d->aaWorld.x, d->aaWorld.y, d->aaWorld.z);
        #endif
//        printf("\n");
    }
}

float Mpu6050Dmp::yaw() const
{
	return d->ypr[0] * 180 / M_PI;
}

float Mpu6050Dmp::pitch() const
{
	return d->ypr[1] * 180 / M_PI;
}

float Mpu6050Dmp::roll() const
{
	return d->ypr[2] * 180 / M_PI;
}
