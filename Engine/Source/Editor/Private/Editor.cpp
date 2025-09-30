#include "pch.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/Axis.h"
#include "Editor/Public/BatchLines.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Editor/Public/SplitterWindow.h"
#include "Editor/Public/Viewport.h"
#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"

IMPLEMENT_CLASS(UEditor, UObject)
UEditor* GEditor = nullptr;
UEditor::UEditor()
{
	RootSplitter = new SSplitterV;
	LeftSplitter = new SSplitterH;
	RightSplitter = new SSplitterH;
	ViewportWindows = new SWindow[4];
    InitializeLayout();

    Gizmo = NewObject<UGizmo>(this);
    Axis = NewObject<UAxis>(this);
    BatchLines = NewObject<UBatchLines>(this);
}

UEditor::~UEditor()
{
	UConfigManager::GetInstance().SetSplitterRatio(RootSplitter->GetRatio(), LeftSplitter->GetRatio(), RightSplitter->GetRatio());

	delete RootSplitter; delete LeftSplitter; delete RightSplitter;
	delete[] ViewportWindows;
    delete Gizmo; delete Axis; delete BatchLines;
}

void UEditor::Update()
{
	UpdateLayout();
}

void UEditor::SetSingleViewportLayout(int32 InActiveIndex)
{
	if (ViewportLayoutState == EViewportLayoutState::Animating) return;

	if (ViewportLayoutState == EViewportLayoutState::Multi)
	{
		SavedRootRatio = RootSplitter->GetRatio();
		SavedLeftRatio = LeftSplitter->GetRatio();
		SavedRightRatio = RightSplitter->GetRatio();
	}

	SourceRootRatio = RootSplitter->GetRatio();
	SourceLeftRatio = LeftSplitter->GetRatio();
	SourceRightRatio = RightSplitter->GetRatio();

	TargetRootRatio = SourceRootRatio;
	TargetLeftRatio = SourceLeftRatio;
	TargetRightRatio = SourceRightRatio;

	switch (InActiveIndex)
	{
	case 0: // 좌상단
		TargetRootRatio = 1.0f;
		TargetLeftRatio = 1.0f;
		break;
	case 1: // 좌하단
		TargetRootRatio = 1.0f;
		TargetLeftRatio = 0.0f;
		break;
	case 2: // 우상단
		TargetRootRatio = 0.0f;
		TargetRightRatio = 1.0f;
		break;
	case 3: // 우하단
		TargetRootRatio = 0.0f;
		TargetRightRatio = 0.0f;
		break;
	default:
		RestoreMultiViewportLayout();
		return;
	}

	ViewportLayoutState = EViewportLayoutState::Animating;
	TargetViewportLayoutState = EViewportLayoutState::Single;
	AnimationStartTime = UTimeManager::GetInstance().GetGameTime();
}

/**
 * @brief 저장된 비율을 사용하여 4분할 뷰포트 레이아웃으로 복원합니다.
 */
void UEditor::RestoreMultiViewportLayout()
{
	if (ViewportLayoutState == EViewportLayoutState::Animating) return;

	SourceRootRatio = RootSplitter->GetRatio();
	SourceLeftRatio = LeftSplitter->GetRatio();
	SourceRightRatio = RightSplitter->GetRatio();

	TargetRootRatio = SavedRootRatio;
	TargetLeftRatio = SavedLeftRatio;
	TargetRightRatio = SavedRightRatio;

	ViewportLayoutState = EViewportLayoutState::Animating;
	TargetViewportLayoutState = EViewportLayoutState::Multi;
	AnimationStartTime = UTimeManager::GetInstance().GetGameTime();
}

void UEditor::RenderViewportOverlay(UCamera* InCamera)
{
	ULevel* Level = ULevelManager::GetInstance().GetCurrentLevel();
	if (AActor* SelectedActor = Level->GetSelectedActor())
	{
		for (const auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
			{
				FVector WorldMin, WorldMax;
				PrimitiveComponent->GetWorldAABB(WorldMin, WorldMax);
				BatchLines->UpdateBoundingBoxVertices(FAABB(WorldMin, WorldMax));
			}
		}
	}

    BatchLines->Render();
    Axis->Render();
    if (InCamera)
    {
        // UEditor는 ULevelManager를 통해 GWorld 상태를 조회
        AActor* SelectedActor = ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor();
        Gizmo->RenderGizmo(SelectedActor, InCamera->GetCameraType(), InCamera->GetLocation());
    }
}

void UEditor::InitializeLayout()
{
	const TArray<float>& SplitterRatio = UConfigManager::GetInstance().GetSplitterRatio();
	RootSplitter->SetRatio(SplitterRatio[0]);
	LeftSplitter->SetRatio(SplitterRatio[1]);
	RightSplitter->SetRatio(SplitterRatio[2]);

	RootSplitter->SetChildren(LeftSplitter, RightSplitter);
	RootSplitter->SetRatio(0);

	// 2. 각 수평 스플리터의 자식으로 뷰포트 윈도우들을 '주소'로 연결합니다.
	LeftSplitter->SetChildren(&ViewportWindows[0], &ViewportWindows[1]);
	LeftSplitter->SetRatio(0);
	RightSplitter->SetChildren(&ViewportWindows[2], &ViewportWindows[3]);
	RightSplitter->SetRatio(0);

	// 3. 초기 레이아웃 계산
	const D3D11_VIEWPORT& ViewportInfo = URenderer::GetInstance().GetDeviceResources()->GetViewportInfo();
	FRect FullScreenRect = { ViewportInfo.TopLeftX, ViewportInfo.TopLeftY, ViewportInfo.Width, ViewportInfo.Height };
	RootSplitter->Resize(FullScreenRect);
}

void UEditor::UpdateLayout()
{
	UInputManager& Input = UInputManager::GetInstance();
	const FPoint MousePosition = { Input.GetMousePosition().X, Input.GetMousePosition().Y };
	bool bIsHoveredOnSplitter = false;

	// 뷰포트를 전환 중이라면 애니메이션을 적용합니다.
	if (ViewportLayoutState == EViewportLayoutState::Animating)
	{
		float ElapsedTime = UTimeManager::GetInstance().GetGameTime() - AnimationStartTime;
		float Alpha = clamp(ElapsedTime / AnimationDuration, 0.0f, 1.0f);

		float NewRootRatio = Lerp(SourceRootRatio, TargetRootRatio, Alpha);
		float NewLeftRatio = Lerp(SourceLeftRatio, TargetLeftRatio, Alpha);
		float NewRightRatio = Lerp(SourceRightRatio, TargetRightRatio, Alpha);

		RootSplitter->SetRatio(NewRootRatio);
		LeftSplitter->SetRatio(NewLeftRatio);
		RightSplitter->SetRatio(NewRightRatio);

		if (Alpha >= 1.0f)
		{
			ViewportLayoutState = TargetViewportLayoutState;
		}
	}

	// 1. 드래그 상태가 아니라면 커서의 상태를 감지합니다.
	if (DraggedSplitter == nullptr)
	{
		if (LeftSplitter->IsHovered(MousePosition) || RightSplitter->IsHovered(MousePosition))
		{
			bIsHoveredOnSplitter = true;
		}
		else if (RootSplitter->IsHovered(MousePosition))
		{
			bIsHoveredOnSplitter = true;
		}
	}

	// 2. 스플리터 위에 커서가 있으며 클릭을 한다면, 드래그 상태로 활성화합니다.
	if (UInputManager::GetInstance().IsKeyPressed(EKeyInput::MouseLeft) && bIsHoveredOnSplitter)
	{
		// 호버 상태에 따라 드래그할 스플리터를 결정합니다.
		if (LeftSplitter->IsHovered(MousePosition)) { DraggedSplitter = LeftSplitter; }				// 좌상, 좌하
		else if (RightSplitter->IsHovered(MousePosition)) { DraggedSplitter = RightSplitter; }		// 우상, 우하
		else if (RootSplitter->IsHovered(MousePosition)) { DraggedSplitter = RootSplitter; }
	}

	// 3. 드래그 상태라면 스플리터 기능을 이행합니다.
	if (DraggedSplitter)
	{
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		FRect WorkableRect = { Viewport->WorkPos.x, Viewport->WorkPos.y, Viewport->WorkSize.x, Viewport->WorkSize.y };

		FRect ParentRect;

		if (DraggedSplitter == RootSplitter)
		{
			ParentRect = WorkableRect;
		}
		else
		{
			if (DraggedSplitter == LeftSplitter)
			{
				ParentRect.Left = WorkableRect.Left;
				ParentRect.Top = WorkableRect.Top;
				ParentRect.Width = WorkableRect.Width * RootSplitter->GetRatio();
				ParentRect.Height = WorkableRect.Height;
			}
			else if (DraggedSplitter == RightSplitter)
			{
				ParentRect.Left = WorkableRect.Left + WorkableRect.Width * RootSplitter->GetRatio();
				ParentRect.Top = WorkableRect.Top;
				ParentRect.Width = WorkableRect.Width * (1.0f - RootSplitter->GetRatio());
				ParentRect.Height = WorkableRect.Height;
			}
		}

		// 마우스 위치를 부모 영역에 대한 비율(0.0 ~ 1.0)로 변환합니다.
		float NewRatio = 0.5f;
		if (dynamic_cast<SSplitterV*>(DraggedSplitter)) // 수직 스플리터
		{
			if (ParentRect.Width > 0)
			{
				NewRatio = (MousePosition.X - ParentRect.Left) / ParentRect.Width;
			}
		}
		else // 수평 스플리터
		{
			if (ParentRect.Height > 0)
			{
				NewRatio = (MousePosition.Y - ParentRect.Top) / ParentRect.Height;
			}
		}

		// 계산된 비율을 스플리터에 적용합니다.
		DraggedSplitter->SetRatio(NewRatio);
	}

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	FRect WorkableRect = { Viewport->WorkPos.x, Viewport->WorkPos.y, Viewport->WorkSize.x, Viewport->WorkSize.y };
	RootSplitter->Resize(WorkableRect);

	// 4. 계산된 뷰포트 영역 목록을 엔진 코어에 전달
	TArray<FRect> CalculatedViewportRects;
	for (int i = 0; i < 4; ++i)
	{
		CalculatedViewportRects.push_back(ViewportWindows[i].Rect);
	}
	GEditorEngine->ApplyViewportLayoutToRenderer(CalculatedViewportRects);

	// 마우스 클릭 해제를 하면 드래그 비활성화
	if (UInputManager::GetInstance().IsKeyReleased(EKeyInput::MouseLeft)) { DraggedSplitter = nullptr; }
}
