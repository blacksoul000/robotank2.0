#include <Wire.h>
#include "I2Cdev.h"
#include "mpu6050_dmp.h"
#include "LowPower.h"
#include <avr/power.h>

#include "pinout.h"
#include "common.h"
#include "engine.h"
#include "chassis_control.h"
#include "tower_control.h"

#define RPI_ADDRESS 0x03

ChassisControl* chassis = nullptr;
TowerControl* tower = nullptr;

constexpr double velocityCoef = 32767.0 / 255;
constexpr double positionCoef = 360.0 / 32767;
//constexpr double turnCoef = 13.6 / 2 / 0.5;  // tracksSeparation / 2 / steeringEfficiency

constexpr float vccCoef = 1.07309486781; // 1.098 * 1000 / 1023;
// 1.098 = (real_vcc * 1023) / (measured_vcc * 1000)
const float dividerCoef = 10.9710144928;  // 100K and 10K
const uint16_t minVoltage = 6500; //mV

uint32_t ms, rpiOnline, lastMs, sendMs, lowVoltageMs = 0;
bool lowVoltage = false;

struct JoyAxes
{
    short x1 = 0;
    short y1 = 0;
    short x2 = 0;
    short y2 = 0;
};

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
} arduinoPkg;

struct RpiPkg
{
  uint8_t powerDown : 1;
  uint8_t light : 1;
  uint8_t enginesPower : 1;
  uint8_t stab : 1;
  uint8_t tracking : 1;
  uint8_t reserve: 3;
  JoyAxes axes;
  float deviationX = 0.0;
  uint16_t crc = 0;
};

void setup() {
  Serial.begin(115200); // start serial for output
  Serial.println("Setup!");

  chassis = new ChassisControl();
  tower = new TowerControl();

  // initialize i2c as master
  Wire.begin();

  pinMode(pinVoltage, INPUT);   // ADC pin
  analogReference(INTERNAL);    // set the ADC reference to 1.1V
  burn8Readings(pinVoltage);    // make 8 readings but don't use them

  // light
  pinMode(pinLeftLight, OUTPUT);
  pinMode(pinRightLight, OUTPUT);

  pinMode(pinEnginesPower, OUTPUT);
  digitalWrite(pinEnginesPower, LOW);

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
    char c = Wire.read();
    if (i < sizeof(data)) 
    {
      data[i] = c;
      ++i;
    }
  }

  if (i < sizeof(RpiPkg)) return;
  
  const RpiPkg* pkg = reinterpret_cast< const RpiPkg* >(data);
  if (!isValid(pkg)) return;

  if (pkg->powerDown) sleepNow();
  rpiOnline = millis();

  digitalWrite(pinEnginesPower, pkg->enginesPower);
  digitalWrite(pinLeftLight, pkg->light);
  digitalWrite(pinRightLight, pkg->light);

  Serial.print(pkg->axes.x1);
  Serial.print("   ");
  Serial.print(pkg->axes.y1);
  Serial.print("   ");
  Serial.print(pkg->axes.x2);
  Serial.print("   ");
  Serial.print(pkg->axes.y2);
  Serial.print("   ");
  Serial.print(pkg->stab);
  Serial.print("   ");
  Serial.print(pkg->tracking);
  Serial.print("   ");
  Serial.println(pkg->deviationX);
  
  tower->setStab(pkg->stab);
  tower->setTracking(pkg->tracking);
  tower->setDeviation(pkg->deviationX);
  
  chassis->move(pkg->axes.x1, pkg->axes.y1);
  tower->move(pkg->axes.x2, pkg->axes.y2);
}

void sendData()
{
  //  http://forum.arduino.cc/index.php?topic=338633.0
  constexpr float currentCoef = vccCoef * 11370 / 1500;  // to convert to mA

  arduinoPkg.currentLeft = chassis->left()->currentValue * currentCoef;
  arduinoPkg.currentRight = chassis->right()->currentValue * currentCoef;
  arduinoPkg.currentTower = tower->engine()->currentValue * currentCoef;

  arduinoPkg.chassisImuOnline = chassis->imu()->isOnline();
  arduinoPkg.chassisImuReady = chassis->imu()->isReady();
  arduinoPkg.towerImuOnline = tower->imu()->isOnline();
  arduinoPkg.towerImuReady = tower->imu()->isReady();

  if (chassis->imu()->isReady())
  {
    // imu is rotated. pitch and roll swapped.
    arduinoPkg.yaw = chassis->imu()->yaw();
    arduinoPkg.pitch = chassis->imu()->roll();
    arduinoPkg.roll = chassis->imu()->pitch();
    if (tower->imu()->isReady())
    {
      arduinoPkg.towerH = tower->position();
    }
  }

  uint16_t batteryValue = analogRead(pinVoltage);    // read actual value
  uint16_t vcc = batteryValue * vccCoef;
  arduinoPkg.voltage = vcc * dividerCoef; // mV
  arduinoPkg.crc = crc16(reinterpret_cast< unsigned char* >(&arduinoPkg), sizeof(ArduinoPkg) - sizeof(arduinoPkg.crc));

  Wire.beginTransmission(RPI_ADDRESS); // transmit to rpi
  Wire.write(reinterpret_cast< const unsigned char* >(&arduinoPkg), sizeof(arduinoPkg));
  Wire.endTransmission();    // stop transmitting

  if (arduinoPkg.voltage < minVoltage)
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

void blinkLight(uint8_t times, uint8_t interval)
{
  for (int i = 0; i < times; ++i)
  {
    digitalWrite(pinLeftLight, LOW);
    digitalWrite(pinRightLight, LOW);
    delay(interval);
    digitalWrite(pinLeftLight, HIGH);
    digitalWrite(pinRightLight, HIGH);
    delay(interval);
  }
}

void sleepNow()
{
  blinkLight(3, 500);

  digitalWrite(pinLeftLight, LOW);
  digitalWrite(pinRightLight, LOW);

  power_all_disable();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // POWER OFF
}

void loop()
{
  ms = millis();

  if (lastMs + 20 < ms)
  {
    lastMs = ms;
    Wire.requestFrom(RPI_ADDRESS, 32);
    processRpiData();
  }
  
  if (rpiOnline + 1000 < ms)
  {
    digitalWrite(pinEnginesPower, LOW);
    digitalWrite(pinLeftPwm, 0);
    digitalWrite(pinRightPwm, 0);
    digitalWrite(pinTowerPwm, 0);
  }

  if (sendMs + 50 < ms)
  {
    sendMs = ms;

    chassis->updateCurrentData();
    chassis->updateImuData();
    
    tower->updateCurrentData();
    tower->updateImuData();
    tower->setChassisYaw(chassis->imu()->yaw());
    sendData();
  }

  if (lowVoltage && lowVoltageMs + 3000 < ms)
  {
    sleepNow();
  }
  
  tower->updatePosition();
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
