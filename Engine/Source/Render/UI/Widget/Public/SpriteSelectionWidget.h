#pragma once
#include "Widget.h"

class UClass;
class UBillBoardComponent;

UClass;
class USpriteSelectionWidget
	: public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(USpriteSelectionWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void PostProcess() override;

	// Special Member Function
	USpriteSelectionWidget();
	~USpriteSelectionWidget() override;

private:
	void UpdateSpriteFromActor();
	void SetSpriteOfActor();

	AActor* SelectedActor;
	FName SelectedSpriteName{""};
};
