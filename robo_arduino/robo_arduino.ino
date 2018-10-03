#include <Wire.h>
#include "I2Cdev.h"
#include "LowPower.h"
#include <avr/power.h>

#define SLAVE_ADDRESS 0x4

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
const int8_t voltagePin = 17; // A3
const int8_t leftLightPin = 2;
const int8_t rightLightPin = 16; // A2
const int8_t enginesPower = 3;

const double velocityCoef = 32767.0 / 255;
const double positionCoef = 360.0 / 32767;

const float vccCoef = 1.07309486781; // 1.098 * 1000 / 1023;
// 1.098 = (real_vcc * 1023) / (measured_vcc * 1000)
const float dividerCoef = 10.9710144928;  // 100K and 10K
const uint16_t minVoltage = 6500; //mV

uint32_t ms, rpiOnline = 0;
bool ledOn = false;

struct RpiPkg
{
    uint8_t shot : 1;
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

  // Engine Left
  pinMode(boardL1, OUTPUT);
  pinMode(boardL2, OUTPUT);
  pinMode(boardPwmL, OUTPUT);
  applySpeed(0, boardL1, boardL2, boardPwmL);

  // Engine Right
  pinMode(boardR1, OUTPUT);
  pinMode(boardR2, OUTPUT);
  pinMode(boardPwmR, OUTPUT);
  applySpeed(0, boardR1, boardR2, boardPwmR);
  
  // Tower
  pinMode(tower1, OUTPUT);
  pinMode(tower2, OUTPUT);
  pinMode(towerPwm, OUTPUT);
  applySpeed(0, tower1, tower2, towerPwm);
  
  // Shot
  pinMode(shotPwm, OUTPUT);
  digitalWrite(shotPwm, 0);
  
  pinMode(voltagePin, INPUT);   // ADC pin
  analogReference(INTERNAL);    // set the ADC reference to 1.1V
  burn8Readings(voltagePin);    // make 8 readings but don't use them

  // light
  pinMode(leftLightPin, OUTPUT);
  pinMode(rightLightPin, OUTPUT);

  // rpi echange indicator  
  pinMode(13, OUTPUT);

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
    while(Wire.available()) Wire.read();
  }
}

void processRpiData()
{
  char data[sizeof(RpiPkg)];
  int i = 0;
  while(Wire.available()) 
  {
    data[i] = (char)Wire.read();
    ++i;
  }

  const RpiPkg* pkg = reinterpret_cast< const RpiPkg* >(data);
  if (!isValid(pkg)) return;
  rpiOnline = millis();
  
  digitalWrite(enginesPower, HIGH);
  digitalWrite(shotPwm, pkg->shot);
  digitalWrite(leftLightPin, pkg->light);
  digitalWrite(rightLightPin, pkg->light);
  
  applySpeed(pkg->leftEngine / velocityCoef, boardL1, boardL2, boardPwmL);
  applySpeed(pkg->rightEngine / velocityCoef, boardR1, boardR2, boardPwmR);
  applySpeed(pkg->towerH / velocityCoef, tower1, tower2, towerPwm);
}

void applySpeed(int16_t speed, int8_t pin1, int8_t pin2, int8_t pinPwm)
{
  int16_t absSpeed = abs(speed);
  analogWrite(pinPwm, min(absSpeed, 255));
  if (speed > 0)
  {
    digitalWrite(pin2, LOW);
    digitalWrite(pin1, HIGH);
  }
  else if (speed < 0)
  {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, HIGH);
  } 
  else
  {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
  }
}

void sendData()
{
  struct ArduinoPkg
  {
      int16_t voltage = 0;
      uint16_t crc = 0;
  } pkg;

  uint16_t batteryValue = analogRead(voltagePin);    // read actual value
  uint16_t vcc = batteryValue * vccCoef;
  pkg.voltage = vcc * dividerCoef; // mV
  pkg.crc = crc16(reinterpret_cast< unsigned char* >(&pkg), sizeof(ArduinoPkg) - sizeof(pkg.crc));
  
  Wire.write(reinterpret_cast< const unsigned char* >(&pkg), sizeof(pkg));
  
  digitalWrite(13, ledOn ? HIGH : LOW);  
  ledOn = !ledOn;
  
  if (pkg.voltage < minVoltage)
  {
    sleepNow();
  }
}

void sleepNow()
{
  // blink 3 times
  for (int i = 0; i <= 3; ++i)
  {
    digitalWrite(13, LOW);
    delay(500);
    digitalWrite(13, HIGH);
    delay(500);
  }
  digitalWrite(13, LOW);

  power_all_disable();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // POWER OFF
}

void loop()
{
  ms = millis();
  if (rpiOnline + 1000 < ms)
  {
    digitalWrite(enginesPower, LOW);
    digitalWrite(boardPwmL, 0);
    digitalWrite(boardPwmR, 0);
    digitalWrite(towerPwm, 0);
    digitalWrite(shotPwm, 0);
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

bool isValid(const RpiPkg* pkg) {
  return pkg->crc == crc16(reinterpret_cast< const unsigned char* > (pkg), sizeof(RpiPkg) - sizeof(pkg->crc));
}
