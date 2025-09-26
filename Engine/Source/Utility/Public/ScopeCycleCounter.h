#pragma once
#include <windows.h>
#include <cstdint>
#include "Global/Types.h"

class FWindowsPlatformTime
{
public:
    static void InitTiming()
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

    static double GetSecondsPerCycle()
    {
        if (!bInitialized)
        {
            InitTiming();
        }
        return GSecondsPerCycle;
    }

    static uint64 GetFrequency()
    {
        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        return Frequency.QuadPart;
    }

    static float ToMilliseconds(uint64 CycleDiff)
    {
        double Ms = static_cast<double>(CycleDiff)
            * GetSecondsPerCycle()
            * 1000.0;

        return (float)Ms;
    }

    static uint64 Cycles64()
    {
        LARGE_INTEGER CycleCount;
        QueryPerformanceCounter(&CycleCount);
        return (uint64)CycleCount.QuadPart;
    }

private:
    static double GSecondsPerCycle;
    static bool bInitialized;
};

double FWindowsPlatformTime::GSecondsPerCycle = 0.0;
bool FWindowsPlatformTime::bInitialized = false;

struct TStatId
{
};

typedef FWindowsPlatformTime FPlatformTime;

class FScopeCycleCounter
{
public:
    FScopeCycleCounter(TStatId StatId)
        : StartCycles(FPlatformTime::Cycles64())
        , UsedStatId(StatId)
    {
    }

    ~FScopeCycleCounter()
    {
        Finish();
    }

    uint64 Finish()
    {
        const uint64 EndCycles = FPlatformTime::Cycles64();
        const uint64 CycleDiff = EndCycles - StartCycles;
        return CycleDiff;
    }

private:
    uint64 StartCycles;
    TStatId UsedStatId;
};
