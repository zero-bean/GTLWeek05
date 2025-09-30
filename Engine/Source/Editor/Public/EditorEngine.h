#pragma once
#include "Core/Public/Object.h"

class FViewport;
class FViewportClient;
class UObjectPicker;
class UTextComponent;

extern class UEditorEngine* GEditorEngine;

class UEditorEngine : public UObject
{
	DECLARE_CLASS(UEditorEngine, UObject)

public:
	UEditorEngine();
	~UEditorEngine();

	virtual void Tick(float DeltaSeconds);
	/**
	* @brief 매 프레임 현재 비율에 맞게 전체 레이아웃 크기를 다시 계산하고, 그 결과를 실제 FViewport에 반영
	* @params ViewportRects 각 Viewport의 Rect들
	*/
	void ApplyViewportLayoutToRenderer(const TArray<FRect>& ViewportRects);

	UTextComponent* GetPickedBillboard() const { return PickedBillboard; }
	FViewport* GetViewportClient() const { return ViewportClient; }

private:
	void ProcessMouseInput(ULevel* InLevel, UGizmo* InGizmo);

	FVector GetGizmoDragLocation(UCamera* InActiveCamera, FRay& WorldRay, UGizmo* InGizmo);
	FVector GetGizmoDragRotation(UCamera* InActiveCamera, FRay& WorldRay, UGizmo* InGizmo);
	FVector GetGizmoDragScale(UCamera* InActiveCamera, FRay& WorldRay, UGizmo* InGizmo);

	FViewport* ViewportClient = nullptr;
	FViewportClient* InteractionViewport = nullptr;
	// -- Editor Objects --
	UObjectPicker* ObjectPicker;
	UTextComponent* PickedBillboard;
};

