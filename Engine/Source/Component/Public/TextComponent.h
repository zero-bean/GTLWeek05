#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"
#include "Physics/Public/AABB.h"

class AActor;
class FAABB;

UCLASS()
class UTextComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UTextComponent, UPrimitiveComponent)

public:
	UTextComponent();
	~UTextComponent();

	virtual void UpdateRotationMatrix(const FVector& InCameraLocation);
	virtual FMatrix GetRTMatrix() const;

	const FString& GetText();
	void SetText(const FString& InText);

	TObjectPtr<UClass> GetSpecificWidgetClass() const override;
private:
	void RegulatePickingAreaByTextLength();

	FString Text = FString("Text");

	FAABB PickingAreaBoundingBox;

	TArray<FNormalVertex> PickingAreaVertex;
	const static inline TArray<uint32> PickingAreaIndex =
	{
		// First Triangle
		0, 1, 2,
		// Second Triangle
		1, 3, 2
	};

public:
	virtual UObject* Duplicate() override;
protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

};
