#include "Utilities.h"
#include <sstream>

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

	std::vector<std::string> Split(std::string& text, const std::string& delimiter)
	{
		std::vector<std::string> splitted;
		size_t pos = 0;
		std::string token;
		while ((pos = text.find(delimiter)) != std::string::npos)
		{
			token = text.substr(0, pos);
			splitted.push_back(token);
			text.erase(0, pos + delimiter.size());
		}
		splitted.push_back(text);

		return splitted;
	}

	bool IsNumber(const std::string& s)
	{
		std::string::const_iterator it = s.begin();
		while (it != s.end() && std::isdigit(*it)) ++it;
		return !s.empty() && it == s.end();
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