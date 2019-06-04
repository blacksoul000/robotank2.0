#include <Wire.h>
#include "I2Cdev.h"
#include "LowPower.h"
#include <avr/power.h>

#define SLAVE_ADDRESS 0x4

struct Engine
{
  int8_t clockwisePin;
  int8_t counterClockwisePin;
  int8_t pwmPin;
  int8_t currentSensorPin;
  int16_t currentValue;
};

Engine left = Engine{8, 7, 9, 15, 0};
Engine right = Engine{5, 4, 6, 14, 0};
Engine tower = Engine{13, 12, 11, 16, 0};

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

uint32_t ms, rpiOnline, lastCurrent, lowVoltageMs = 0;
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

void setup() {
  Serial.begin(115200); // start serial for output

  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);

  // define callbacks for i2c communication
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  Engine engines[] = {&left, &right, &tower};
  for (auto& eng : engines)
  {
    pinMode(eng.clockwisePin, OUTPUT);
    pinMode(eng.counterClockwisePin, OUTPUT);
    pinMode(eng.pwmPin, OUTPUT);
    pinMode(eng.currentSensorPin, INPUT);
    digitalWrite(eng.pwmPin, 0);
  }

  pinMode(voltagePin, INPUT);   // ADC pin
  analogReference(INTERNAL);    // set the ADC reference to 1.1V
  burn8Readings(voltagePin);    // make 8 readings but don't use them

  // light
  pinMode(leftLightPin, OUTPUT);
  pinMode(rightLightPin, OUTPUT);

  pinMode(enginesPower, OUTPUT);
  digitalWrite(enginesPower, LOW);

  // Serial.println("Ready!");
  delay(10);
}

// callback for received data
void receiveData(int byteCount)
{
  if (byteCount == sizeof(RpiPkg))
  {
    processRpiData();
  }
  else
  {
    // Garbage? Can't handle, drop.
    while (Wire.available()) Wire.read();
  }
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

  applySpeed(pkg->leftEngine / velocityCoef, left);
  applySpeed(pkg->rightEngine / velocityCoef, right);
  applySpeed(pkg->towerH / velocityCoef, tower);
}

void applySpeed(int16_t speed, const Engine& engine)
{
  int16_t absSpeed = abs(speed);
  if (speed > 0)
  {
    digitalWrite(engine.counterClockwisePin, LOW);
    digitalWrite(engine.clockwisePin, HIGH);
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
  analogWrite(engine.pwmPin, min(absSpeed, 255));
}

void sendData()
{
  struct ArduinoPkg
  {
    int16_t voltage = 0;
    int16_t currentLeft;
    int16_t currentRight;
    int16_t currentTower;
    uint16_t crc = 0;
  } pkg;

  //  http://forum.arduino.cc/index.php?topic=338633.0
  constexpr float currentCoef = vccCoef * 11370 / 1500;  // to convert to mA

  pkg.currentLeft = left.currentValue * currentCoef;
  pkg.currentRight = right.currentValue * currentCoef;
  pkg.currentTower = tower.currentValue * currentCoef;

  uint16_t batteryValue = analogRead(voltagePin);    // read actual value
  uint16_t vcc = batteryValue * vccCoef;
  pkg.voltage = vcc * dividerCoef; // mV
  pkg.crc = crc16(reinterpret_cast< unsigned char* >(&pkg), sizeof(ArduinoPkg) - sizeof(pkg.crc));

  Wire.write(reinterpret_cast< const unsigned char* >(&pkg), sizeof(pkg));

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
  Serial.println("Ready!");
  ms = millis();
  if (rpiOnline + 1000 < ms)
  {
    digitalWrite(enginesPower, LOW);
    digitalWrite(left.pwmPin, 0);
    digitalWrite(right.pwmPin, 0);
    digitalWrite(tower.pwmPin, 0);
  }

  if (lastCurrent + 100 < ms)
  {
    lastCurrent = ms;
    left.currentValue = analogRead(left.currentSensorPin);
    right.currentValue = analogRead(right.currentSensorPin);
    tower.currentValue = analogRead(tower.currentSensorPin);
  }

  if (lowVoltage && lowVoltageMs + 3000 < ms)
  {
//    blinkLight(20, 500);
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
