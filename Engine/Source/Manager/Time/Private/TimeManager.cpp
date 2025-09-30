#include "pch.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Utility/Public/ScopeCycleCounter.h"

IMPLEMENT_CLASS(UTimeManager, UObject)
IMPLEMENT_SINGLETON(UTimeManager)

UTimeManager::UTimeManager()
{
	Initialize();
}

UTimeManager::~UTimeManager() = default;

void UTimeManager::Initialize()
{
	GameTime = 0.0f;
	DeltaTime = 0.0f;
	bIsPaused = false;
}

void UTimeManager::Update()
{
	if (!bIsPaused)
	{
		GameTime += DeltaTime;
	}
}
