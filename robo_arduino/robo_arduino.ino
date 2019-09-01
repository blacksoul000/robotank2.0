#include <Wire.h>
#include "I2Cdev.h"
#include "mpu6050_dmp.h"
#include "LowPower.h"
#include <avr/power.h>

#define RPI_ADDRESS 0x03

struct Engine
{
  int8_t clockwisePin;
  int8_t counterClockwisePin;
  int8_t pwmPin;
  int8_t currentSensorPin;
  int16_t currentValue;
};

struct ImuData
{
  IImu* imu = nullptr;
  bool ready = false;
  double yawOffset = 0;
};

Engine left = Engine{8, 7, 9, 15, 0};
Engine right = Engine{5, 4, 6, 14, 0};
Engine tower = Engine{13, 12, 11, 16, 0};

ImuData chassisImu;
ImuData towerImu;

const int8_t voltagePin = 17; // A3
const int8_t leftLightPin = 3;
const int8_t rightLightPin = 2;
const int8_t enginesPower = 10;

const double velocityCoef = 32767.0 / 255;
const double positionCoef = 360.0 / 32767;

constexpr float vccCoef = 1.07309486781; // 1.098 * 1000 / 1023;
// 1.098 = (real_vcc * 1023) / (measured_vcc * 1000)
const float dividerCoef = 10.9710144928;  // 100K and 10K
const uint16_t minVoltage = 6500; //mV

uint32_t ms, rpiOnline, lastMs, sendMs, lowVoltageMs = 0;
bool lowVoltage = false;

struct RpiPkg
{
  uint8_t powerDown : 1;
  uint8_t light : 1;
  uint8_t reserve: 6;
  int16_t leftEngine = 0;
  int16_t rightEngine = 0;
  int16_t towerH = 0;
  uint16_t crc = 0;
};

void initEngine(const Engine& engine)
{
  Serial.print("Init engine: ");
  Serial.print(engine.clockwisePin);
  Serial.print("   ");
  Serial.print(engine.counterClockwisePin);
  Serial.print("   ");
  Serial.print(engine.pwmPin);
  Serial.print("   ");
  Serial.println(engine.currentSensorPin);

  pinMode(engine.clockwisePin, OUTPUT);
  pinMode(engine.counterClockwisePin, OUTPUT);
  pinMode(engine.pwmPin, OUTPUT);
  pinMode(engine.currentSensorPin, INPUT);
  digitalWrite(engine.pwmPin, 0);
}

void setup() {
  Serial.begin(115200); // start serial for output

  // initialize i2c as master
  Wire.begin();

  // define callbacks for i2c communication
  Wire.onReceive(receiveData);
  //  Wire.onRequest(sendData);

  towerImu.imu = new Mpu6050Dmp(0x69, 14, -53, 25, 825, 304, 1254);
  chassisImu.imu = new Mpu6050Dmp(0x68, 68, -17, -31, -2364, -983, 987);

  if (!towerImu.imu->init())
  {
    Serial.println("Failed to init 'tower' IMU");
  }
  if (!chassisImu.imu->init())
  {
    Serial.println("Failed to init 'chassis' IMU");
  }

  initEngine(left);
  initEngine(right);
  initEngine(tower);

  pinMode(voltagePin, INPUT);   // ADC pin
  analogReference(INTERNAL);    // set the ADC reference to 1.1V
  burn8Readings(voltagePin);    // make 8 readings but don't use them

  // light
  pinMode(leftLightPin, OUTPUT);
  pinMode(rightLightPin, OUTPUT);

  pinMode(enginesPower, OUTPUT);
  digitalWrite(enginesPower, LOW);

  Serial.println("Ready!");
  delay(10);
}

// callback for received data
void receiveData(int byteCount)
{
  Serial.print("Received bytes: ");
  Serial.println(byteCount);
  processRpiData();
}

void processRpiData()
{
  char data[sizeof(RpiPkg)];
  int i = 0;
  while (Wire.available())
  {
    data[i] = (char)Wire.read();
    ++i;
  }

  const RpiPkg* pkg = reinterpret_cast< const RpiPkg* >(data);
  if (!isValid(pkg)) return;

  if (pkg->powerDown) sleepNow();
  rpiOnline = millis();

  digitalWrite(enginesPower, HIGH);
  digitalWrite(leftLightPin, pkg->light);
  digitalWrite(rightLightPin, pkg->light);

  //  Serial.print(pkg->leftEngine / velocityCoef);
  //  Serial.print("   ");
  //  Serial.print(pkg->rightEngine / velocityCoef);
  //  Serial.print("   ");
  //  Serial.println(pkg->towerH / velocityCoef);

  applySpeed(pkg->leftEngine / velocityCoef, left);
  applySpeed(pkg->rightEngine / velocityCoef, right);
  applySpeed(pkg->towerH / velocityCoef, tower);
}

void applySpeed(int16_t speed, const Engine& engine)
{
  if (speed > 0)
  {
    digitalWrite(engine.clockwisePin, HIGH);
    digitalWrite(engine.counterClockwisePin, LOW);
  }
  else if (speed < 0)
  {
    digitalWrite(engine.clockwisePin, LOW);
    digitalWrite(engine.counterClockwisePin, HIGH);
  }
  else
  {
    digitalWrite(engine.clockwisePin, LOW);
    digitalWrite(engine.counterClockwisePin, LOW);
  }
  analogWrite(engine.pwmPin, min(abs(speed), 255));
}

void sendData()
{
  struct ArduinoPkg
  {
    int16_t voltage = 0;
    int16_t currentLeft;
    int16_t currentRight;
    int16_t currentTower;
    float yaw = 0;
    float pitch = 0;
    float roll = 0;
    float towerH = 0;
    uint8_t chassisImuOnline : 1;
    uint8_t chassisImuReady : 1;
    uint8_t towerImuOnline : 1;
    uint8_t towerImuReady : 1;
    uint8_t reserve : 4;
    uint16_t crc = 0;
  } pkg;

  //  http://forum.arduino.cc/index.php?topic=338633.0
  constexpr float currentCoef = vccCoef * 11370 / 1500;  // to convert to mA

  pkg.currentLeft = left.currentValue * currentCoef;
  pkg.currentRight = right.currentValue * currentCoef;
  pkg.currentTower = tower.currentValue * currentCoef;

  pkg.chassisImuOnline = chassisImu.imu->isOnline();
  pkg.chassisImuReady = chassisImu.imu->isReady();
  pkg.towerImuOnline = chassisImu.imu->isOnline();
  pkg.towerImuReady = chassisImu.imu->isReady();

  if (chassisImu.ready)
  {
    // imu is rotated. pitch and roll swapped.
    pkg.yaw = chassisImu.imu->yaw() - chassisImu.yawOffset;
    pkg.pitch = chassisImu.imu->roll();
    pkg.roll = chassisImu.imu->pitch();
    if (towerImu.ready)
    {
      pkg.towerH = towerImu.imu->yaw() - towerImu.yawOffset - pkg.yaw;
    }
  }

  uint16_t batteryValue = analogRead(voltagePin);    // read actual value
  uint16_t vcc = batteryValue * vccCoef;
  pkg.voltage = vcc * dividerCoef; // mV
  pkg.crc = crc16(reinterpret_cast< unsigned char* >(&pkg), sizeof(ArduinoPkg) - sizeof(pkg.crc));

  Wire.beginTransmission(RPI_ADDRESS); // transmit to rpi
  Wire.write(reinterpret_cast< const unsigned char* >(&pkg), sizeof(pkg));
  Wire.endTransmission();    // stop transmitting

  if (pkg.voltage < minVoltage)
  {
    if (!lowVoltage)
    {
      lowVoltageMs = ms;
      lowVoltage = true;
    }
  } else {
    lowVoltage = false;
  }
}

void readGyroData(ImuData& imuData)
{
  imuData.imu->readData();

  if (imuData.imu->isReady() != imuData.ready)
  {
    imuData.ready = imuData.imu->isReady();
    imuData.yawOffset = imuData.imu->yaw();
  }
}

void blinkLight(uint8_t times, uint8_t interval)
{
  for (int i = 0; i < times; ++i)
  {
    digitalWrite(leftLightPin, LOW);
    digitalWrite(rightLightPin, LOW);
    delay(interval);
    digitalWrite(leftLightPin, HIGH);
    digitalWrite(rightLightPin, HIGH);
    delay(interval);
  }
}

void sleepNow()
{
  blinkLight(3, 500);

  digitalWrite(leftLightPin, LOW);
  digitalWrite(rightLightPin, LOW);

  power_all_disable();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // POWER OFF
}

void loop()
{
  ms = millis();
  if (rpiOnline + 1000 < ms)
  {
    digitalWrite(enginesPower, LOW);
    digitalWrite(left.pwmPin, 0);
    digitalWrite(right.pwmPin, 0);
    digitalWrite(tower.pwmPin, 0);
  }

  if (lastMs + 20 < ms)
  {
    lastMs = ms;
    left.currentValue = analogRead(left.currentSensorPin);
    right.currentValue = analogRead(right.currentSensorPin);
    tower.currentValue = analogRead(tower.currentSensorPin);
    Wire.requestFrom(RPI_ADDRESS, sizeof(RpiPkg));
  }

  if (sendMs + 50 < ms)
  {
    sendMs = ms;
    readGyroData(chassisImu);
    readGyroData(towerImu);
    sendData();
  }

  if (lowVoltage && lowVoltageMs + 3000 < ms)
  {
    sleepNow();
  }
}

void burn8Readings(int pin)
{
  for (int i = 0; i < 8; i++)
  {
    analogRead(pin);
  }
}

uint16_t crc16(const unsigned char* data, unsigned short len)
{
  unsigned short crc = 0xFFFF;
  unsigned char i;

  while (len--)
  {
    crc ^= *data++ << 8;

    for (i = 0; i < 8; i++)
      crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
  }
  return crc;
}

bool isValid(const RpiPkg* pkg)
{
  return pkg->crc == crc16(reinterpret_cast< const unsigned char* > (pkg), sizeof(RpiPkg) - sizeof(pkg->crc));
}
