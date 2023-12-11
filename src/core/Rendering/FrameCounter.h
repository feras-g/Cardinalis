#pragma once

#include <Windows.h>
#include "core/engine/logger.h"
#include <array>

class Timestep
{
public:
	Timestep() = default;
	Timestep(float time) : m_DeltaTimeSeconds(time) {}

	inline float GetSeconds()      const { return m_DeltaTimeSeconds; };
	inline float GetMilliseconds() const { return m_DeltaTimeSeconds * 1000.0f; };
protected:
	float m_DeltaTimeSeconds = 0.0f;
};

constexpr size_t MaxFrames = 8;

class FrameStats
{
public:
	FrameStats() = delete;
	static inline float AccumulatedFrameTime = 0.0f;
	static inline float CurrentFrameTimeAvg = 0.0f;
	static inline std::array<float, MaxFrames> History{};

	static void UpdateFrameTimeHistory(float currentFrameTime)
	{
		History[CurrentPosition] = currentFrameTime;
		AccumulatedFrameTime += currentFrameTime;
		CurrentPosition = (CurrentPosition + 1) % MaxFrames;
		if (CurrentPosition == 0)
		{
			CurrentFrameTimeAvg = AccumulatedFrameTime / MaxFrames;
			AccumulatedFrameTime = 0.0f;
		}
	}
private:
	static inline size_t CurrentPosition = 0;
};
