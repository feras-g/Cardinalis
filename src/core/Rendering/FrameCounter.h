#pragma once

class FrameCounter
{
public:
	explicit FrameCounter() : m_AvgIntervalSeconds(0.5f), m_CurrentFps(0.0f), m_AccumulatedTime(0.0), m_NumFrames(0), m_CpuDeltaSeconds(0.0f) {}
	bool Tick(float deltaSeconds, bool frameRendered = true);
	
	inline float GetFps() { return m_CurrentFps; }
	inline float GetCpuDeltaSeconds() { return m_CpuDeltaSeconds; }
	inline double GetAccumulatedTime() { return m_AccumulatedTime; }
private:
	float m_AvgIntervalSeconds;
	float m_CurrentFps;
	float m_CpuDeltaSeconds;
	double m_AccumulatedTime;
	unsigned int m_NumFrames;
};