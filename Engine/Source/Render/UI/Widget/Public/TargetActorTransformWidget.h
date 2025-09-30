#pragma once
#include "Widget.h"

class UTargetActorTransformWidget : public UWidget
{
    GENERATED_BODY()
    DECLARE_CLASS(UTargetActorTransformWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void PostProcess() override;

	void UpdateTransformFromActor();
	void ApplyTransformToActor() const;

	// Special Member Function
	UTargetActorTransformWidget();
	~UTargetActorTransformWidget() override;

private:
	AActor* SelectedActor;

	FVector EditLocation;
	FVector EditRotation;
	FVector EditScale;
	bool bScaleChanged;
	bool bRotationChanged;
	bool bPositionChanged;
	uint64 LevelMemoryByte;
	uint32 LevelObjectCount;
};
