#include <Servo.h>
#include <Wire.h>
#include "I2Cdev.h"

#include "MPU6050_6Axis_MotionApps20.h"

//#define ENABLE_GYRO
//#define ENABLE_SERVO

// chassis engines
const int8_t boardL1 = 8;
const int8_t boardL2 = 7;
const int8_t boardPwmL = 11;
const int8_t boardR1 = 5;
const int8_t boardR2 = 4;
const int8_t boardPwmR = 6;
const int8_t tower1 = 15;
const int8_t tower2 = 14;
const int8_t towerPwm = 10;

const int8_t gunPwm = 17;
const int8_t cameraPwm = 16;
const int8_t shotPwm = 9;

// mpu
struct MpuData
{
    volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
    bool dmpReady = false;  // set true if DMP init was successful
    uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
    uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
    uint16_t fifoCount;     // count of all bytes currently in FIFO
    uint8_t fifoBuffer[64]; // FIFO storage buffer
    Quaternion q;           // [w, x, y, z]         quaternion container
    VectorFloat gravity;    // [x, y, z]            gravity vector
    float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
};

const double velocityCoef = 32767.0 / 255;
const double positionCoef = 360.0 / 32767;

struct RpiPkg
{
    uint8_t angleType:3;
    uint8_t shot:1;
    uint8_t reserve:4;
    int16_t gunV = 0;
    int16_t cameraV = 0;
    int16_t leftEngine = 0;
    int16_t rightEngine = 0;
    int16_t towerH = 0;
};

Servo gun;
Servo camera;

MPU6050 mpu[] = {0x69, 0x68};
MpuData mpuData[2];

uint32_t ms, ms1, ms2 = 0;
int8_t gunSpeed = 0;

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
  
  // Servo
#ifdef ENABLE_SERVO
  gun.attach(gunPwm);
  camera.attach(cameraPwm);
#endif
  
#ifdef ENABLE_GYRO
  mpu[0].initialize();
  mpu[1].initialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu[0].setXGyroOffset(43);
  mpu[0].setYGyroOffset(-52);
  mpu[0].setZGyroOffset(27);
  mpu[0].setXAccelOffset(888);
  mpu[0].setYAccelOffset(248);
  mpu[0].setZAccelOffset(1274);
  mpu[1].setXGyroOffset(59);
  mpu[1].setYGyroOffset(-15);
  mpu[1].setZGyroOffset(-28);
  mpu[1].setXAccelOffset(-2389);
  mpu[1].setYAccelOffset(-796);
  mpu[1].setZAccelOffset(988);

  setupGyro(0);
  setupGyro(1);

  // enable Arduino interrupt detection
  // Serial.println(F("Enabling interrupt detection (Arduino external interrupt)..."));
  attachInterrupt(digitalPinToInterrupt(2), dmpDataReady0, RISING);
  attachInterrupt(digitalPinToInterrupt(3), dmpDataReady1, RISING);
#endif

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  
//  Serial.println("Ready!");
}

void dmpDataReady0() { mpuData[0].mpuInterrupt = true; }
void dmpDataReady1() { mpuData[1].mpuInterrupt = true; }

void setupGyro(uint8_t index)
{
    mpu[index].setFullScaleGyroRange(MPU6050_GYRO_FS_2000);

    uint8_t devStatus = mpu[index].dmpInitialize();
    // make sure it worked (returns 0 if so)
    if (devStatus == 0)
    {
      // turn on the DMP, now that it's ready
  //    Serial.println(F("Enabling DMP..."));
      mpu[index].setDMPEnabled(true);

      mpuData[index].mpuIntStatus = mpu[index].getIntStatus();

      // set our DMP Ready flag so the main loop() function knows it's okay to use it
  //    Serial.println(F("DMP ready! Waiting for first interrupt..."));
      mpuData[index].dmpReady = true;

      // get expected DMP packet size for later comparison
      mpuData[index].packetSize = mpu[index].dmpGetFIFOPacketSize();
    }
    else
    {
      // ERROR!
      // 1 = initial memory load failed
      // 2 = DMP configuration updates failed
      // (if it's going to break, usually the code will be 1)
      Serial.print(F("DMP Initialization failed (code "));
      Serial.print(devStatus);
      Serial.println(F(")"));
    }
}

void processAccelGyro(uint8_t index)
{
    MPU6050& currentMpu = mpu[index];
    MpuData& currentData = mpuData[index];

  // if programming failed, don't try to do anything
  if (!currentData.dmpReady) return;

  // wait for MPU interrupt or extra packet(s) available
  if (!currentData.mpuInterrupt && currentData.fifoCount < currentData.packetSize) return;

  // reset interrupt flag and get INT_STATUS byte
  currentData.mpuInterrupt = false;
  currentData.mpuIntStatus = currentMpu.getIntStatus();

  // get current FIFO count
  currentData.fifoCount = currentMpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((currentData.mpuIntStatus & 0x10) || currentData.fifoCount == 1024) {
    // reset so we can continue cleanly
    currentMpu.resetFIFO();
    //    Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
  }
  else if (currentData.mpuIntStatus & 0x02) {
    // wait for correct available data length, should be a VERY short wait
    while (currentData.fifoCount < currentData.packetSize) currentData.fifoCount = currentMpu.getFIFOCount();

    // read a packet from FIFO
    currentMpu.getFIFOBytes(currentData.fifoBuffer, currentData.packetSize);

    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    currentData.fifoCount -= currentData.packetSize;

    // display Euler angles in degrees
    currentMpu.dmpGetQuaternion(&currentData.q, currentData.fifoBuffer);

    currentMpu.dmpGetGravity(&currentData.gravity, &currentData.q);
    currentMpu.dmpGetYawPitchRoll(currentData.ypr, &currentData.q, &currentData.gravity);
//    Serial.print("ypr\t");
//    Serial.print(ypr[0] * 180 / M_PI);
//    Serial.print("\t");
//    Serial.print(ypr[1] * 180 / M_PI);
//    Serial.print("\t");
//    Serial.println(ypr[2] * 180 / M_PI);
  }
}  // processAccelGyro()

void applySpeed(int16_t speed, int8_t pin1, int8_t pin2, int8_t pinPwm)
{
  int16_t absSpeed = abs(speed);
  analogWrite(pinPwm, absSpeed > 255 ? 255 : absSpeed);
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
      int16_t gunX = 0;
      int16_t gunY = 0;
      int16_t cameraY = 0;
      int16_t yaw = 0;
      int16_t pitch = 0;
      int16_t roll = 0;
  } pkg;

  pkg.gunX = mpuData[0].ypr[0] * 180 / M_PI / positionCoef;
  pkg.gunY = gun.read() / positionCoef;
  pkg.cameraY = camera.read() / positionCoef;
  pkg.yaw = mpuData[1].ypr[0] /** 180 / M_PI / positionCoef*/;
  pkg.pitch = -mpuData[1].ypr[2] /** 180 / M_PI / positionCoef*/; //bcoz sensor attitude pitch and roll swapped and pitch inverted
  pkg.roll = mpuData[1].ypr[1] /** 180 / M_PI / positionCoef*/;

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
    waitPrefix = true;
    buf.remove(0, packetSize);

    // debug start    
    mpuData[1].ypr[0] = pkg.leftEngine / velocityCoef;
    mpuData[1].ypr[1] = pkg.towerH / velocityCoef;
    mpuData[1].ypr[2] = -pkg.rightEngine / velocityCoef;
    // debug end
    digitalWrite(shotPwm, pkg.shot);
    applySpeed(pkg.leftEngine / velocityCoef, boardL1, boardL2, boardPwmL);
    applySpeed(pkg.rightEngine / velocityCoef, boardR1, boardR2, boardPwmR);
    applySpeed(pkg.towerH / velocityCoef, tower1, tower2, towerPwm);

#ifdef ENABLE_SERVO
    if (pkg.angleType == 1) // position
    {
        gunSpeed = 0;
        gun.write(gun.read() - pkg.gunV * (90.0 / 32767));
        camera.write(camera.read() - pkg.cameraV * (90.0 / 32767));
    }
    else
    {
        gunSpeed = 1.0 * pkg.gunV / 32767 * 3;
    }
#endif
  }
}

void loop()
{
  ms = millis();
  if ((ms - ms1) > 50 || ms < ms1 )
  {
    ms1 = ms;

    sendData();
  }

  if ((ms - ms2) > 30 || ms < ms2 )
  {
    ms2 = ms;
#ifdef ENABLE_GYRO
    processAccelGyro(0);
    processAccelGyro(1);
#endif

#ifdef ENABLE_SERVO
    gun.write(gun.read() - gunSpeed);
    camera.write(camera.read() - gunSpeed);
#endif    
  }
}
