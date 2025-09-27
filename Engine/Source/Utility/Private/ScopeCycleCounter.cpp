#include "pch.h"
#include "Utility/Public/ScopeCycleCounter.h"

void FWindowsPlatformTime::InitTiming()
{
    if (!bInitialized)
    {
        bInitialized = true;

        double Frequency = (double)GetFrequency();
        if (Frequency <= 0.0)
        {
            Frequency = 1.0;
        }

        GSecondsPerCycle = 1.0 / Frequency;
    }
}

double FWindowsPlatformTime::GetSecondsPerCycle()
{
    if (!bInitialized)
    {
        InitTiming();
    }
    return GSecondsPerCycle;
}

uint64 FWindowsPlatformTime::GetFrequency()
{
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);
    return Frequency.QuadPart;
}


float FWindowsPlatformTime::ToMilliseconds(uint64 CycleDiff)
{
    double Ms = static_cast<double>(CycleDiff)
        * GetSecondsPerCycle()
        * 1000.0;

    return (float)Ms;
}

uint64 FWindowsPlatformTime::Cycles64()
{
    LARGE_INTEGER CycleCount;
    QueryPerformanceCounter(&CycleCount);
    return (uint64)CycleCount.QuadPart;
}

double FWindowsPlatformTime::GSecondsPerCycle = 0.0;
bool FWindowsPlatformTime::bInitialized = false;
