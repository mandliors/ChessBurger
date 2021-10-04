#include "Utilities.h"

namespace Utils
{
	std::string Round(float num, int decimals)
	{
		std::stringstream ss;
		ss.precision(decimals);
		ss << num;
		std::string s = ss.str();
		int idx = s.find('.');
		int diff;
		if (idx == -1)
		{
			s += ".";
			diff = decimals;
		}
		else
			diff = decimals - s.size() + 1 + idx;

		for (int i = 0; i < diff; i++)
			s += "0";

		return s;
	}
}

namespace Math
{
	float Map(float x, float min1, float max1, float min2, float max2)
	{
		float slope = (max2 - min2) / (max1 - min1);
		float y = min2 + slope * (x - min1);
		return y;
	}

	float Clamp(float x, float min, float max)
	{
		return x < min ? min : (x > max ? max : x);
	}
}