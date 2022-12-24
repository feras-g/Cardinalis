#include "FrameCounter.h"
#include <stdio.h>

bool FrameCounter::Tick(float deltaSeconds, bool frameRendered)
{
    m_CpuDeltaSeconds = deltaSeconds;
    if (frameRendered) 
    {
        m_NumFrames++;
    }

    m_AccumulatedTime += deltaSeconds;
    
    if (m_AccumulatedTime < m_AvgIntervalSeconds)
    {
        return false;
    }

    m_CurrentFps = m_NumFrames / (float)m_AccumulatedTime;

    m_NumFrames = 0;
    m_AccumulatedTime = 0.0f;
    return true;
}   
