#include "pch.h"
#include "Editor/Public/EditorEngine.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Editor/Public/Viewport.h"
#include "Editor/Public/ViewportClient.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/public/Axis.h"
#include "Editor/Public/BatchLines.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Level/Public/Level.h"

IMPLEMENT_CLASS(UEditorEngine, UObject)
UEditorEngine* GEditorEngine = nullptr;
UEditorEngine::UEditorEngine()
{
	ViewportClient = new FViewport();
	ObjectPicker = NewObject<UObjectPicker>(this);
}

UEditorEngine::~UEditorEngine()
{
	delete ViewportClient;
    delete ObjectPicker;
}

void UEditorEngine::Tick(float DeltaSeconds)
{
	// 1. 마우스 위치를 기반으로 활성 뷰포트를 결정합니다.
	ViewportClient->UpdateActiveViewportClient(UInputManager::GetInstance().GetMousePosition());

	// 2. 활성 뷰포트의 카메라의 제어만 업데이트합니다.
	if (UCamera* ActiveCamera = ViewportClient->GetActiveCamera())
	{
		// 만약 이동량이 있고, 직교 카메라라면 ViewportClient에 알립니다.
		const FVector MovementDelta = ActiveCamera->UpdateInput();
		if (MovementDelta.LengthSquared() > 0.f && ActiveCamera->GetCameraType() == ECameraType::ECT_Orthographic)
		{
			ViewportClient->UpdateOrthoFocusPointByDelta(MovementDelta);
		}
	}

    UBatchLines* BatchLines = GEditor->GetBatchLines();
    UGizmo* Gizmo = GEditor->GetGizmo();

	if (AActor* SelectedActor = ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor())
	{
		for (const auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
			{
				FVector WorldMin, WorldMax;
				PrimitiveComponent->GetWorldAABB(WorldMin, WorldMax);

				uint64 ShowFlags = ULevelManager::GetInstance().GetCurrentLevel()->GetShowFlags();

				if ((ShowFlags & EEngineShowFlags::SF_Primitives) && (ShowFlags & EEngineShowFlags::SF_Bounds))
				{
					UE_LOG("AABB: Min(%f, %f, %f), Max(%f, %f, %f)",
	WorldMin.X, WorldMin.Y, WorldMin.Z,
	WorldMax.X, WorldMax.Y, WorldMax.Z);

					BatchLines->UpdateBoundingBoxVertices(FAABB(WorldMin, WorldMax));
				}
				else
				{
					BatchLines->UpdateBoundingBoxVertices({ { 0.0f,0.0f,0.0f }, { 0.0f, 0.0f, 0.0f } });
				}
			}
		}
	}
	else
	{
		BatchLines->DisableRenderBoundingBox();
	}

	BatchLines->UpdateVertexBuffer();
    ProcessMouseInput(ULevelManager::GetInstance().GetCurrentLevel(), Gizmo);
}

void UEditorEngine::ProcessMouseInput(ULevel* InLevel, UGizmo* InGizmo)
{
	FViewportClient* CurrentViewport = nullptr;

	// 이미 선택된 뷰포트 영역이 존재한다면 선택된 뷰포트 처리를 진행
    if (InteractionViewport) { CurrentViewport = InteractionViewport; }
	// 선택된 뷰포트 영역이 존재하지 않는다면 현재 마우스 위치의 뷰포트를 선택
	else { CurrentViewport = ViewportClient->GetActiveViewportClient(); }

	// 처리할 영역이 존재하지 않으면 진행을 중단
	if (CurrentViewport == nullptr) { return; }

	UCamera* CurrentCamera = CurrentViewport->Camera;
	TObjectPtr<AActor> ActorPicked = InLevel->GetSelectedActor();

	if (ActorPicked)
	{
		// 피킹 전 현재 카메라에 맞는 기즈모 스케일 업데이트
		InGizmo->UpdateScale(CurrentCamera);
		// 빌보드 갱신
		PickedBillboard = ActorPicked->GetBillBoardComponent();
	}
	else
	{
		PickedBillboard = nullptr;
	}

	const UInputManager& InputManager = UInputManager::GetInstance();
	const FVector& MousePos = InputManager.GetMousePosition();
	const D3D11_VIEWPORT& ViewportInfo = CurrentViewport->GetViewportInfo();

	const float NdcX = ((MousePos.X - ViewportInfo.TopLeftX) / ViewportInfo.Width) * 2.0f - 1.0f;
	const float NdcY = -(((MousePos.Y - ViewportInfo.TopLeftY) / ViewportInfo.Height) * 2.0f - 1.0f);

	FRay WorldRay = CurrentCamera->ConvertToWorldRay(NdcX, NdcY);

	static EGizmoDirection PreviousGizmoDirection = EGizmoDirection::None;
	FVector CollisionPoint;
	float ActorDistance = -1;

	if (InputManager.IsKeyPressed(EKeyInput::Tab))
	{
		InGizmo->IsWorldMode() ? InGizmo->SetLocal() : InGizmo->SetWorld();
	}
	if (InputManager.IsKeyPressed(EKeyInput::Space))
	{
		InGizmo->ChangeGizmoMode();
	}
	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		InGizmo->EndDrag();
		// 드래그가 끝나면 선택된 뷰포트를 비활성화 합니다.
		InteractionViewport = nullptr;
	}

	if (InGizmo->IsDragging() && InGizmo->GetSelectedActor())
	{
		switch (InGizmo->GetGizmoMode())
		{
		    case EGizmoMode::Translate:
		    {
			    FVector GizmoDragLocation = GetGizmoDragLocation(CurrentCamera, WorldRay, InGizmo);
			    InGizmo->SetLocation(GizmoDragLocation);
			    break;
		    }
		    case EGizmoMode::Rotate:
		    {
			    FVector GizmoDragRotation = GetGizmoDragRotation(CurrentCamera, WorldRay, InGizmo);
			    InGizmo->SetActorRotation(GizmoDragRotation);
			    break;
		    }
		    case EGizmoMode::Scale:
		    {
			    FVector GizmoDragScale = GetGizmoDragScale(CurrentCamera, WorldRay, InGizmo);
			    InGizmo->SetActorScale(GizmoDragScale);
		    }
		}
	}
	else
	{
		if (InLevel->GetSelectedActor() && InGizmo->HasActor())
		{
			ObjectPicker->PickGizmo(CurrentCamera, WorldRay, InGizmo, CollisionPoint);
		}
		else
		{
			InGizmo->SetGizmoDirection(EGizmoDirection::None);
		}

		if (!ImGui::GetIO().WantCaptureMouse && InputManager.IsKeyPressed(EKeyInput::MouseLeft))
		{
			if (ULevelManager::GetInstance().GetCurrentLevel()->GetShowFlags() & EEngineShowFlags::SF_Primitives)
			{
				TArray<UPrimitiveComponent*> Candidate;

				ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
				ObjectPicker->FindCandidateFromOctree(CurrentLevel->GetStaticOctree(), WorldRay, Candidate);

				TArray<UPrimitiveComponent*>& DynamicCandidates = CurrentLevel->GetDynamicPrimitives();
				if (!DynamicCandidates.empty())
				{
					Candidate.insert(Candidate.end(), DynamicCandidates.begin(), DynamicCandidates.end());
				}


				TStatId StatId("Picking");
				FScopeCycleCounter PickCounter(StatId);

				UPrimitiveComponent* PrimitiveCollided = ObjectPicker->PickPrimitive(CurrentCamera, WorldRay, Candidate, &ActorDistance);
				ActorPicked = PrimitiveCollided ? PrimitiveCollided->GetOwner() : nullptr;
				double ElapsedMs = PickCounter.Finish();

				UStatOverlay::GetInstance().RecordPickingStats(ElapsedMs);
			}
		}

		if (InGizmo->GetGizmoDirection() == EGizmoDirection::None)
		{
			InLevel->SetSelectedActor(ActorPicked);
			if (PreviousGizmoDirection != EGizmoDirection::None)
			{
				InGizmo->OnMouseRelease(PreviousGizmoDirection);
			}
		}
		else
		{
			PreviousGizmoDirection = InGizmo->GetGizmoDirection();
			if (InputManager.IsKeyPressed(EKeyInput::MouseLeft))
			{
				InGizmo->OnMouseDragStart(CollisionPoint);
				// 드래그가 활성화하면 뷰포트를 고정합니다.
				InteractionViewport = CurrentViewport;
			}
			else
			{
				InGizmo->OnMouseHovering();
			}
		}
	}
}

void UEditorEngine::ApplyViewportLayoutToRenderer(const TArray<FRect>& ViewportRects)
{
	TArray<FViewportClient>& Viewports = ViewportClient->GetViewports();

	// 인자로 받은 ViewportRects 배열을 순회하며 실제 뷰포트 정보에 적용합니다.
	for (int i = 0; i < ViewportRects.size() && i < Viewports.size(); ++i)
	{
		const FRect& Rect = ViewportRects[i];
		Viewports[i].SetViewportInfo({ Rect.Left, Rect.Top, Rect.Width, Rect.Height, 0.0f, 1.0f });
	}
}

FVector UEditorEngine::GetGizmoDragLocation(UCamera* InActiveCamera, FRay& WorldRay, UGizmo* InGizmo)
{
	FVector MouseWorld;
	FVector PlaneOrigin = InGizmo->GetGizmoLocation() ;
	FVector GizmoAxis = InGizmo->GetGizmoAxis();

	if (!InGizmo->IsWorldMode())
	{
		FVector4 GizmoAxis4{ GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f };
		FVector RadRotation = FVector::GetDegreeToRadian(InGizmo->GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	if (ObjectPicker->IsRayCollideWithPlane(WorldRay, PlaneOrigin, InActiveCamera->CalculatePlaneNormal(GizmoAxis).Cross(GizmoAxis), MouseWorld))
	{
		FVector MouseDistance = MouseWorld - InGizmo->GetDragStartMouseLocation();
		return InGizmo->GetDragStartActorLocation() + GizmoAxis * MouseDistance.Dot(GizmoAxis);
	}
	return InGizmo->GetGizmoLocation();
}

FVector UEditorEngine::GetGizmoDragRotation(UCamera* InActiveCamera, FRay& WorldRay, UGizmo* InGizmo)
{
	FVector MouseWorld;
	FVector PlaneOrigin = InGizmo->GetGizmoLocation();
	FVector GizmoAxis = InGizmo->GetGizmoAxis();

	if (!InGizmo->IsWorldMode())
	{
		FVector4 GizmoAxis4{ GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f };
		FVector RadRotation = FVector::GetDegreeToRadian(InGizmo->GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	if (ObjectPicker->IsRayCollideWithPlane(WorldRay, PlaneOrigin, GizmoAxis, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = InGizmo->GetDragStartMouseLocation() - PlaneOrigin;
		PlaneOriginToMouse.Normalize();
		PlaneOriginToMouseStart.Normalize();
		float DotResult = (PlaneOriginToMouseStart).Dot(PlaneOriginToMouse);
		float Angle = acosf(std::max(-1.0f, std::min(1.0f, DotResult)));
		if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(GizmoAxis) < 0)
		{
			Angle = -Angle;
		}

		FQuaternion StartRotQuat = FQuaternion::FromEuler(InGizmo->GetDragStartActorRotation());
		FQuaternion DeltaRotQuat = FQuaternion::FromAxisAngle(InGizmo->GetGizmoAxis(), Angle);
		if (InGizmo->IsWorldMode())
		{
			return (DeltaRotQuat * StartRotQuat).ToEuler();
		}
		else
		{
			return (StartRotQuat * DeltaRotQuat).ToEuler();
		}
	}
	return InGizmo->GetActorRotation();
}

FVector UEditorEngine::GetGizmoDragScale(UCamera* InActiveCamera, FRay& WorldRay, UGizmo* InGizmo)
{
	FVector MouseWorld;
	FVector PlaneOrigin = InGizmo->GetGizmoLocation();
	FVector CardinalAxis = InGizmo->GetGizmoAxis();

	FVector4 GizmoAxis4{ CardinalAxis.X, CardinalAxis.Y, CardinalAxis.Z, 0.0f };
	FVector RadRotation = FVector::GetDegreeToRadian(InGizmo->GetActorRotation());
	FVector GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);

	FVector PlaneNormal = InActiveCamera->CalculatePlaneNormal(GizmoAxis).Cross(GizmoAxis);
	if (ObjectPicker->IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = InGizmo->GetDragStartMouseLocation() - PlaneOrigin;
		float DragStartAxisDistance = PlaneOriginToMouseStart.Dot(GizmoAxis);
		float DragAxisDistance = PlaneOriginToMouse.Dot(GizmoAxis);
		float ScaleFactor = 1.0f;
		if (abs(DragStartAxisDistance) > 0.1f)
		{
			ScaleFactor = DragAxisDistance / DragStartAxisDistance;
		}

		FVector DragStartScale = InGizmo->GetDragStartActorScale();
		constexpr float MinScale = 0.1f;
		if (ScaleFactor > MinScale)
		{
			if (InGizmo->GetSelectedActor()->IsUniformScale())
			{
				float UniformValue = DragStartScale.Dot(CardinalAxis);
				return FVector(1.0f, 1.0f, 1.0f) * UniformValue * ScaleFactor;
			}
			else
			{
				return DragStartScale + CardinalAxis * (ScaleFactor - 1.0f) * DragStartScale.Dot(CardinalAxis);
			}
		}
		return InGizmo->GetActorScale();
	}
	return InGizmo->GetActorScale();
}
