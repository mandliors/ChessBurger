#pragma once

#include <sstream>

namespace Utils
{
	std::string Round(float num, int decimals);
}

namespace Math
{
	float Map(float x, float min1, float max1, float min2, float max2);
	float Clamp(float x, float min, float max);
}