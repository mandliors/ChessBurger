#pragma once

#include <string>

class Clock
{
public:
	Clock();
	Clock(double maxSeconds);

	void Update();

	void Start();
	void Pause();
	void Reset();

	void SetMaxSeconds(double maxSecs);

	double GetSecondsLeft() const;
	std::string GetString() const;

private:
	bool m_Started;
	double m_MaxSeconds;
	double m_ElapsedSeconds;
	double m_PreviousTime;
};