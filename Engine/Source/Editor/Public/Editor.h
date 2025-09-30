#pragma once
#include "Core/Public/Object.h"

class UPrimitiveComponent;
class UTextComponent;
class FViewportClient;
class UCamera;
class ULevel;
class USplitterWidget;
struct FRay; 
class UObjectPicker;
class SSplitter; class SSplitterV; class SSplitterH;
class SWindow;
class UGizmo;
class UAxis;
class UBatchLines;


enum class EViewModeIndex : uint32
{
	VMI_Lit,
	VMI_Unlit,
	VMI_Wireframe,
};

enum class EViewportLayoutState
{
	Multi,
	Single,
	Animating,
};

extern class UEditor* GEditor;

class UEditor : public UObject
{
	DECLARE_CLASS(UEditor, UObject)

public:
	UEditor();
	~UEditor();

	void Update();

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }

	void SetSingleViewportLayout(int32 InActiveIndex);
	void RestoreMultiViewportLayout();

	SSplitterV* GetRootSplitter() const { return RootSplitter; }
	SSplitterH* GetLeftSplitter() const { return LeftSplitter; }
	SSplitterH* GetRightSplitter() const { return RightSplitter; }

    UGizmo* GetGizmo() const { return Gizmo; }
    UAxis* GetAxis() const { return Axis; }
    UBatchLines* GetBatchLines() const { return BatchLines; }

    void RenderViewportOverlay(class UCamera* InCamera);
    
private:
	void InitializeLayout();
	void UpdateLayout();

	float Lerp(const float A, const float B, const float Alpha) { return A * (1 - Alpha) + B * Alpha; }

	// -- Viewport Slate --
	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_Lit;

	SSplitterV* RootSplitter;
	SSplitterH* LeftSplitter;
	SSplitterH* RightSplitter;
	SWindow* ViewportWindows;

	SSplitter* DraggedSplitter = nullptr;

	// Viewport Animation
	EViewportLayoutState ViewportLayoutState = EViewportLayoutState::Multi;
	EViewportLayoutState TargetViewportLayoutState = EViewportLayoutState::Multi;
	float AnimationStartTime = 0.0f;
	float AnimationDuration = 0.2f; 
	float SourceRootRatio = 0.5f;
	float SourceLeftRatio = 0.5f;
	float SourceRightRatio = 0.5f;
	float TargetRootRatio = 0.5f;
	float TargetLeftRatio = 0.5f;
	float TargetRightRatio = 0.5f;
	float SavedRootRatio = 0.5f;
	float SavedLeftRatio = 0.5f;
	float SavedRightRatio = 0.5f;

    UGizmo* Gizmo;
    UAxis* Axis;
    UBatchLines* BatchLines;
};
