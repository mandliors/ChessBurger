#pragma once

#include <string>
#include <vector>

namespace Utils
{
	std::string Round(float num, int decimals);
	std::vector<std::string> Split(std::string& text, const std::string& delim);
	bool IsNumber(const std::string& s);
	int _Mbox();
}

namespace Math
{
	float Map(float x, float min1, float max1, float min2, float max2);
	float Clamp(float x, float min, float max);
}