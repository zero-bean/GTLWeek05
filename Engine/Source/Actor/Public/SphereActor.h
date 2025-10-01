#pragma once

#include "Actor/Public/Actor.h"

class USphereComponent;

UCLASS()
class ASphereActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ASphereActor, AActor)

public:
	ASphereActor();

	// AActor의 PostDuplicate를 재정의하여 SphereComponent 포인터를 수정합니다.
	virtual void PostDuplicate(const TMap<UObject*, UObject*>& InDuplicationMap) override;

private:
	USphereComponent* SphereComponent = nullptr;
};
