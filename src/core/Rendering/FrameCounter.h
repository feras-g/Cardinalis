#pragma once

#include <Windows.h>

class Timestep
{
public:
	Timestep(float time) : m_DeltaTimeSeconds(time) {}

	inline float GetSeconds()      const { return m_DeltaTimeSeconds; };
	inline float GetMilliseconds() const { return m_DeltaTimeSeconds * 1000.0f; };
protected:
	float m_DeltaTimeSeconds = 0.0f;
};