#ifndef PINOUT_H
#define PINOUT_H

#include <stdint.h>

// left engine
const int8_t pinLeftCw = 8;   // Clockwise
const int8_t pinLeftCcw = 7;  // CounterClockwise
const int8_t pinLeftPwm = 9;  // Pwm
const int8_t pinLeftCs = 15;  // CurrentSensor

// right engine
const int8_t pinRightCw = 5;
const int8_t pinRightCcw = 4;
const int8_t pinRightPwm = 6;
const int8_t pinRightCs = 14;

// tower
const int8_t pinTowerCw = 13;
const int8_t pinTowerCcw = 12;
const int8_t pinTowerPwm = 11;
const int8_t pinTowerCs = 16;

const int8_t pinVoltage = 17; // A3
const int8_t pinLeftLight = 3;
const int8_t pinRightLight = 2;
const int8_t pinEnginesPower = 10;

#endif // PINOUT_H
