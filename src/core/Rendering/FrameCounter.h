#pragma once

#include <Windows.h>

static double frequency = 0.0f;

class Timestep
{
public:
	Timestep(float time) : m_Time(time) {}

	inline float GetSeconds() const { return m_Time; };
	inline float GetMilliseconds() const { return m_Time * 1000.0f; };
protected:
	// Time in seconds
	float m_Time = 0.0f;
};