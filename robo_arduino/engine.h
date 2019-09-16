#ifndef ENGINE_H
#define ENGINE_H

struct Engine
{
  int8_t clockwisePin;
  int8_t counterClockwisePin;
  int8_t pwmPin;
  int8_t currentSensorPin;
  int16_t currentValue;

  void printPins()
  {
	Serial.print("Init engine: ");
	Serial.print(clockwisePin);
	Serial.print("   ");
	Serial.print(counterClockwisePin);
	Serial.print("   ");
	Serial.print(pwmPin);
	Serial.print("   ");
	Serial.println(currentSensorPin);
  }

  void init()
  {
    printPins();

	pinMode(clockwisePin, OUTPUT);
	pinMode(counterClockwisePin, OUTPUT);
	pinMode(pwmPin, OUTPUT);
	pinMode(currentSensorPin, INPUT);
	digitalWrite(pwmPin, 0);
  }

  void applySpeed(int16_t speed)
  {
    if (speed > 0)
    {
      digitalWrite(clockwisePin, HIGH);
      digitalWrite(counterClockwisePin, LOW);
    }
    else if (speed < 0)
    {
      digitalWrite(clockwisePin, LOW);
      digitalWrite(counterClockwisePin, HIGH);
    }
    else
    {
      digitalWrite(clockwisePin, LOW);
      digitalWrite(counterClockwisePin, LOW);
    }
    analogWrite(pwmPin, min(abs(speed), 255));
  }
};

#endif // ENGINE_H
