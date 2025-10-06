#pragma once
#include "Actor.h"

class UCubeComponent;

UCLASS()
class ADecalActor : public AActor
{
    GENERATED_BODY()
    DECLARE_CLASS(ADecalActor, AActor)

public:
    ADecalActor();
    virtual ~ADecalActor();

    void InitializeComponents() override;

    UClass* GetDefaultRootComponent() override;

private:
    UCubeComponent* VisualizationComponent = nullptr;
};