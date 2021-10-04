#include "Clock.h"
#include "raylib.h"
#include <string>
#include <cmath>

Clock::Clock()
	: m_Started(false), m_MaxSeconds(0.0f), m_ElapsedSeconds(0.0f), m_PreviousTime(GetTime()) { }

Clock::Clock(double maxSeconds)
	: m_Started(false), m_MaxSeconds(maxSeconds), m_ElapsedSeconds(0.0f), m_PreviousTime(GetTime()) { }

void Clock::Update()
{
	if (m_Started)
	{
		double currentTime = GetTime();
		m_ElapsedSeconds += currentTime - m_PreviousTime;
		m_PreviousTime = GetTime();
	}
}

void Clock::Start()
{
	if (!m_Started)
	{
		m_Started = true;
		m_PreviousTime = GetTime();
	}
}

void Clock::Pause()
{
	m_Started = false;
}

void Clock::Reset()
{
	m_ElapsedSeconds = 0.0;
}

void Clock::SetMaxSeconds(double maxSecs)
{
	m_MaxSeconds = maxSecs;
}

double Clock::GetSecondsLeft() const
{
	return m_MaxSeconds - m_ElapsedSeconds;
}

std::string Clock::GetString() const
{
	double secs = m_MaxSeconds - m_ElapsedSeconds;
	uint32_t minutes = std::floor(secs / 60.0);
	uint32_t seconds = std::round(secs - minutes * 60);

	std::string result = "";
	result += std::to_string(minutes);
	result += ":";
	if (seconds < 10)
		result += "0";
	result += std::to_string(seconds);

	return result;
}