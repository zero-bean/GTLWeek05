#pragma once
#include <windows.h>
#include <cstdint>
#include "Global/Types.h"

class FWindowsPlatformTime
{
public:
    static void InitTiming();

    static double GetSecondsPerCycle();

    static uint64 GetFrequency();

    static float ToMilliseconds(uint64 CycleDiff);

    static uint64 Cycles64();

private:
    static double GSecondsPerCycle;
    static bool bInitialized;
};

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
