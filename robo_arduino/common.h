#ifndef COMMON_H
#define COMMON_H

#include "Arduino.h"

namespace common
{
	inline bool fuzzyCompare(float a, float b, float eps)
	{
		return fabs(a - b) < eps;
	}

	inline int bound(int minValue, int value, int maxValue)
	{
		return min(max(value, minValue), maxValue);
	}

	inline double smooth(double value, double maxInputValue, double maxOutputValue)
	{
		return pow((value / maxInputValue), 3) * maxOutputValue;
	}
} // namespace common

#endif // COMMON_H
