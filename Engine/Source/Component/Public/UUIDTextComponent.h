#pragma once
#include "Component/Public/TextComponent.h"
#include "Global/Matrix.h"

class AActor;

UCLASS()
class UUUIDTextComponent : public UTextComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UUUIDTextComponent, UTextComponent)
public:
	UUUIDTextComponent();
	UUUIDTextComponent(AActor* InOwnerActor, float InYOffset);
	~UUUIDTextComponent();

	virtual void OnSelected() override;
	virtual void OnDeselected() override;

	void UpdateRotationMatrix(const FVector& InCameraLocation) override;

	FMatrix GetRTMatrix() const override { return RTMatrix; }

	TObjectPtr<UClass> GetSpecificWidgetClass() const override;
private:
	FMatrix RTMatrix;
	AActor* POwnerActor;
	float ZOffset;
};