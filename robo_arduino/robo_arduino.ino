#include <Wire.h>
#include "I2Cdev.h"

#define ENABLE_GYRO
#define DMP_FIFO_RATE 20

#include "MPU6050_6Axis_MotionApps20.h"

// chassis engines
const int8_t boardL1 = 8;
const int8_t boardL2 = 7;
const int8_t boardPwmL = 9;
const int8_t boardR1 = 5;
const int8_t boardR2 = 4;
const int8_t boardPwmR = 6;
const int8_t tower1 = 15;
const int8_t tower2 = 14;
const int8_t towerPwm = 10;
const int8_t shotPwm = 11;

const double velocityCoef = 32767.0 / 255;
const double positionCoef = 360.0 / 32767;

struct RpiPkg
{
    uint8_t shot = 0;
    int16_t leftEngine = 0;
    int16_t rightEngine = 0;
    int16_t towerH = 0;
};

struct MpuData
{
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
} mpuData;

MPU6050 mpu;

uint32_t ms, ms1, rpiOnline = 0;

String prefix;
String buf;
bool waitPrefix = true;

void setup() {
  Serial.begin(115200); // start serial for output

  prefix += char(0x55);
  prefix += char(0x55);

  // Engine Left
  pinMode(boardL1, OUTPUT);
  pinMode(boardL2, OUTPUT);
  pinMode(boardPwmL, OUTPUT);
  digitalWrite(boardPwmL, 0);

  // Engine Right
  pinMode(boardR1, OUTPUT);
  pinMode(boardR2, OUTPUT);
  pinMode(boardPwmR, OUTPUT);
  digitalWrite(boardPwmR, 0);
  
  // Tower
  pinMode(tower1, OUTPUT);
  pinMode(tower2, OUTPUT);
  pinMode(towerPwm, OUTPUT);
  digitalWrite(towerPwm, 0);
  
  // Shot
  pinMode(shotPwm, OUTPUT);
  digitalWrite(shotPwm, 0);
  
#ifdef ENABLE_GYRO
  Wire.begin();
  initMpu();
#endif

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  
//  Serial.println("Ready!");
}

void processAccelGyro()
{
    // if programming failed, don't try to do anything
    if (!mpuData.dmpReady) return;

    // get current FIFO count
    mpuData.fifoCount = mpu.getFIFOCount();
    
    if (mpuData.fifoCount == 1024)
    {
        // reset so we can continue cleanly
        mpu.resetFIFO();
        // printf("FIFO overflow!\n");
    
    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    }
    else if (mpuData.fifoCount >= 42)
    {
        // read a packet from FIFO
        mpu.getFIFOBytes(mpuData.fifoBuffer, mpuData.packetSize);
    
        // display Euler angles in degrees
        mpu.dmpGetQuaternion(&mpuData.q, mpuData.fifoBuffer);
        mpu.dmpGetGravity(&mpuData.gravity, &mpuData.q);
        mpu.dmpGetYawPitchRoll(mpuData.ypr, &mpuData.q, &mpuData.gravity);
        // printf("ypr  %7.2f %7.2f %7.2f    ", mpuData.ypr[0] * 180/M_PI, mpuData.ypr[1] * 180/M_PI, mpuData.ypr[2] * 180/M_PI);
        
        #ifdef OUTPUT_READABLE_REALACCEL
            // display real acceleration, adjusted to remove gravity
            mpu.dmpGetQuaternion(&mpuData.q, mpuData.fifoBuffer);
            mpu.dmpGetAccel(&mpuData.aa, mpuData.fifoBuffer);
            mpu.dmpGetGravity(&mpuData.gravity, &mpuData.q);
            mpu.dmpGetLinearAccel(&mpuData.aaReal, &mpuData.aa, &mpuData.gravity);
            // printf("areal %6d %6d %6d    ", mpuData.aaReal.x, mpuData.aaReal.y, mpuData.aaReal.z);
        #endif

        #ifdef OUTPUT_READABLE_WORLDACCEL
            // display initial world-frame acceleration, adjusted to remove gravity
            // and rotated based on known orientation from quaternion
            mpu.dmpGetQuaternion(&mpuData.q, mpuData.fifoBuffer);
            mpu.dmpGetAccel(&mpuData.aa, mpuData.fifoBuffer);
            mpu.dmpGetGravity(&mpuData.gravity, &mpuData.q);
            mpu.dmpGetLinearAccelInWorld(&mpuData.aaWorld, &mpuData.aaReal, &mpuData.q);
            // printf("aworld %6d %6d %6d    ", mpuData.aaWorld.x, mpuData.aaWorld.y, mpuData.aaWorld.z);
        #endif
        // printf("\n");
    }
}  // processAccelGyro()

void applySpeed(int16_t speed, int8_t pin1, int8_t pin2, int8_t pinPwm)
{
  int16_t absSpeed = abs(speed);
  analogWrite(pinPwm, min(absSpeed, 255));
  if (speed > 0)
  {
    digitalWrite(pin2, LOW);
    digitalWrite(pin1, HIGH);
  }
  else
  {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, HIGH);
  }
}

void sendData()
{
  struct ArduinoPkg
  {
      int16_t yaw = 0;
      int16_t pitch = 0;
      int16_t roll = 0;
  } pkg;

  pkg.yaw = mpuData.ypr[0] * 180 / M_PI / positionCoef;
  pkg.pitch = mpuData.ypr[2] * 180 / M_PI / positionCoef; //bcoz sensor attitude pitch and roll swapped
  pkg.roll = mpuData.ypr[1] * 180 / M_PI / positionCoef;

  Serial.write(prefix.c_str(), prefix.length());
  Serial.write(reinterpret_cast< const unsigned char* >(&pkg), sizeof(pkg));
}

void serialEvent()
{
  while (Serial.available())
  {
    buf += (char)Serial.read();
  }

  if (waitPrefix)
  {
    if (buf.length() < prefix.length()) return;

    int8_t pos = buf.indexOf(prefix);
    if (pos == -1)
    {
      buf.remove(0, buf.length() - prefix.length());
      return;
    }
    else
    {
      buf.remove(0, pos);
      waitPrefix = false;
    }
  }
  if (!waitPrefix)
  {
    const uint8_t packetSize = prefix.length() + sizeof(RpiPkg);
    if (buf.length() < packetSize) return;

    RpiPkg pkg = *reinterpret_cast< const RpiPkg* >(buf.c_str() + prefix.length());
    rpiOnline = millis();
    waitPrefix = true;
    buf.remove(0, packetSize);

    digitalWrite(shotPwm, pkg.shot);
    applySpeed(pkg.leftEngine / velocityCoef, boardL1, boardL2, boardPwmL);
    applySpeed(pkg.rightEngine / velocityCoef, boardR1, boardR2, boardPwmR);
    applySpeed(pkg.towerH / velocityCoef, tower1, tower2, towerPwm);
  }
}

void loop()
{
  ms = millis();
  if (ms1 + 100 < ms)
  {
    ms1 = ms;
#ifdef ENABLE_GYRO      
    processAccelGyro();
#endif
    sendData();
  }
  if (rpiOnline + 1000 < ms)
  {
    digitalWrite(boardPwmL, 0);
    digitalWrite(boardPwmR, 0);
    digitalWrite(towerPwm, 0);
    digitalWrite(shotPwm, 0);
  }
}

void initMpu()
{
  // initialize device
  // printf("Initializing I2C devices...\n");
  mpu.initialize();

  // verify connection
  // printf("Testing device connections...\n");
  // printf(mpu.testConnection() ? "MPU6050 connection successful\n" : "MPU6050 connection failed\n");

  // load and configure the DMP
  // printf("Initializing DMP...\n");
  mpuData.devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (mpuData.devStatus == 0)
  {
      // turn on the DMP, now that it's ready
      // printf("Enabling DMP...\n");
      mpu.setDMPEnabled(true);

      // set our DMP Ready flag so the main loop() function knows it's okay to use it
      // printf("DMP ready!\n");
      mpuData.dmpReady = true;

      // get expected DMP packet size for later comparison
      mpuData.packetSize = mpu.dmpGetFIFOPacketSize();
  }
  else
  {
      // ERROR!
      // 1 = initial memory load failed
      // 2 = DMP configuration updates failed
      // (if it's going to break, usually the code will be 1)
      // printf("DMP Initialization failed (code %d)\n", mpuData.devStatus);
  }
  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXGyroOffset(68);
  mpu.setYGyroOffset(-17);
  mpu.setZGyroOffset(-31);
  mpu.setXAccelOffset(-2364);
  mpu.setYAccelOffset(-983);
  mpu.setZAccelOffset(987);

//  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
//  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_2000);
}