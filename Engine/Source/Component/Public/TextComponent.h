#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

class AActor;

class UTextComponent : public UPrimitiveComponent
{
public:
	UTextComponent(AActor* InOwnerActor, float InYOffset);
	~UTextComponent();

	virtual void OnSelected() override;
	virtual void OnDeselected() override;

	void UpdateRotationMatrix(const FVector& InCameraLocation);

	FMatrix GetRTMatrix() const { return RTMatrix; }
private:
	FMatrix RTMatrix;
	AActor* POwnerActor;
	float ZOffset;
};
