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
private:
	FString Text = FString("Text");
};
