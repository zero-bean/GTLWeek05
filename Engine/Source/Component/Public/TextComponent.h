#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

class AActor;

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
	
public:
	virtual UObject* Duplicate() override;
	
protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;
	
private:
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
};
