#include "pch.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/TextComponent.h"
#include "Component/Public/BillBoardComponent.h"
#include "Component/Mesh/Public/SphereComponent.h"
#include "Component/Mesh/Public/SquareComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/TriangleComponent.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Global/Quaternion.h"

UActorDetailWidget::UActorDetailWidget()
	: UWidget("Actor Detail Widget")
{
}

UActorDetailWidget::~UActorDetailWidget() = default;

void UActorDetailWidget::Initialize()
{
	UE_LOG("ActorDetailWidget: Initialized");
}

void UActorDetailWidget::Update()
{
	// 특별한 업데이트 로직이 필요하면 여기에 추가
}

void UActorDetailWidget::RenderWidget()
{
	TObjectPtr<ULevel> CurrentLevel = GWorld->GetLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	TObjectPtr<AActor> SelectedActor = CurrentLevel->GetSelectedActor();
	if (!SelectedActor)
	{
		ImGui::TextUnformatted("No Object Selected");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Detail 확인을 위해 Object를 선택해주세요");
		SelectedComponent = nullptr; 
		CachedSelectedActor = nullptr;
		return;
	}

	// 선택된 액터가 변경되면, 컴포넌트 선택 상태를 초기화
	if (CachedSelectedActor != SelectedActor)
	{
		SelectedComponent = nullptr;
		CachedSelectedActor = SelectedActor;
	}

	// Actor 헤더 렌더링 (이름 + rename 기능)
	RenderActorHeader(SelectedActor);

	ImGui::Separator();

	// 컴포넌트 트리 렌더링
	RenderComponentTree(SelectedActor);

	ImGui::Separator();

	// 선택된 컴포넌트의 트랜스폼 정보 렌더링
	RenderTransformEdit();
}

void UActorDetailWidget::RenderActorHeader(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor)
	{
		return;
	}

	FName ActorName = InSelectedActor->GetName();
	FString ActorDisplayName = ActorName.ToString();

	ImGui::Text("[A]");
	ImGui::SameLine();

	if (bIsRenamingActor)
	{
		// Rename 모드
		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("##ActorRename", ActorNameBuffer, sizeof(ActorNameBuffer),
		                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			FinishRenamingActor(InSelectedActor);
		}

		// ESC로 취소, InputManager보다 일단 내부 API로 입력 받는 것으로 처리
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			CancelRenamingActor();
		}
	}
	else
	{
		// 더블클릭으로 rename 시작
		ImGui::Text("%s", ActorDisplayName.data());

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			StartRenamingActor(InSelectedActor);
		}

		// 툴팁 UI
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Double-Click to Rename");
		}
	}
}

/**
 * @brief 컴포넌트들을 트리 형태로 표시하는 함수
 * @param InSelectedActor 선택된 Actor
 */
void UActorDetailWidget::RenderComponentTree(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor) return;

	const auto& Components = InSelectedActor->GetOwnedComponents();

	ImGui::Text("Components (%d)", static_cast<int>(Components.size()));
	RenderAddComponentButton(InSelectedActor);
	ImGui::Separator();

	if (Components.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No components");
		return;
	}

	// 모든 컴포넌트 중 부모가 없는 최상위 컴포넌트만 찾아 재귀 렌더링을 시작합니다.
	for (const auto& Component : Components)
	{
		if (!Component) continue;

		USceneComponent* SceneComp = Cast<USceneComponent>(Component.Get());
		// SceneComponent가 아니거나, 부모가 없는 SceneComponent가 최상위입니다.
		if (!SceneComp || !SceneComp->GetParentAttachment())
		{
			RenderComponentNodeRecursive(Component.Get());
		}
	}
}

void UActorDetailWidget::RenderComponentNodeRecursive(UActorComponent* InComponent)
{
	if (!InComponent) return;

	USceneComponent* SceneComponent = Cast<USceneComponent>(InComponent);
	FString ComponentName = InComponent->GetName().ToString();

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (!SceneComponent || SceneComponent->GetChildren().empty())
		NodeFlags |= ImGuiTreeNodeFlags_Leaf;
	if (SelectedComponent == InComponent)
		NodeFlags |= ImGuiTreeNodeFlags_Selected;

	// Use the component's pointer as a unique ID for the tree node.
	bool bNodeOpen = ImGui::TreeNodeEx((void*)InComponent, NodeFlags, "%s", ComponentName.data());

	if (ImGui::IsItemClicked())
	{
		SelectedComponent = InComponent;
	}

	if (bNodeOpen)
	{
		if (SceneComponent)
		{
			for (auto& Child : SceneComponent->GetChildren())
			{
				RenderComponentNodeRecursive(Child);
			}
		}
		ImGui::TreePop();
	}
}

void UActorDetailWidget::RenderAddComponentButton(TObjectPtr<AActor> InSelectedActor)
{
	ImGui::SameLine();

	const char* buttonText = "[+]";
	float buttonWidth = ImGui::CalcTextSize(buttonText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonWidth);

	if (ImGui::Button(buttonText))
	{
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		ImGui::Text("Add Component");
		ImGui::Separator();

		const char* componentNames[] = {
			"Triangle", "Sphere", "Square", "Cube",
			"Mesh", "Static Mesh", "BillBoard", "Text"
		};

		// 반복문 안에서 헬퍼 함수를 호출하여 원하는 UI를 그립니다.
		for (const char* name : componentNames)
		{
			if (CenteredSelectable(name))
			{
				AddComponentByName(InSelectedActor, FString(name));
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}
}

bool UActorDetailWidget::CenteredSelectable(const char* label)
{
	// 1. 라벨이 보이지 않는 Selectable을 전체 너비로 만들어 호버링/클릭 영역을 잡습니다.
	bool clicked = ImGui::Selectable(std::string("##").append(label).c_str());

	// 2. 방금 만든 Selectable의 사각형 영역 정보를 가져옵니다.
	ImVec2 minRect = ImGui::GetItemRectMin();
	ImVec2 maxRect = ImGui::GetItemRectMax();

	// 3. 텍스트 크기를 계산합니다.
	ImVec2 textSize = ImGui::CalcTextSize(label);

	// 4. 텍스트를 그릴 중앙 위치를 계산합니다.
	ImVec2 textPos = ImVec2(
		minRect.x + (maxRect.x - minRect.x - textSize.x) * 0.5f,
		minRect.y + (maxRect.y - minRect.y - textSize.y) * 0.5f
	);

	// 5. 계산된 위치에 텍스트를 직접 그립니다.
	ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), label);

	return clicked;
}

void UActorDetailWidget::AddComponentByName(TObjectPtr<AActor> InSelectedActor, const FString& InComponentName)
{
	if (!InSelectedActor)
	{
		UE_LOG_WARNING("ActorDetailWidget: 컴포넌트를 추가할 액터가 선택되지 않았습니다.");
		return;
	}

	FName NewComponentName(InComponentName);
	UActorComponent* NewComponent = nullptr; 

	if (InComponentName == "Triangle")
	{
		NewComponent = InSelectedActor->AddComponent<UTriangleComponent>(NewComponentName);
	}
	else if (InComponentName == "Sphere")
	{
		NewComponent = InSelectedActor->AddComponent<USphereComponent>(NewComponentName);
	}
	else if (InComponentName == "Square")
	{
		NewComponent = InSelectedActor->AddComponent<USquareComponent>(NewComponentName);
	}
	else if (InComponentName == "Cube")
	{
		NewComponent = InSelectedActor->AddComponent<UCubeComponent>(NewComponentName);
	}
	else if (InComponentName == "Mesh")
	{
		NewComponent = InSelectedActor->AddComponent<UMeshComponent>(NewComponentName);
	}
	else if (InComponentName == "Static Mesh")
	{
		NewComponent = InSelectedActor->AddComponent<UStaticMeshComponent>(NewComponentName);
	}
	else if (InComponentName == "BillBoard")
	{
		NewComponent = InSelectedActor->AddComponent<UBillBoardComponent>(NewComponentName);
	}
	else if (InComponentName == "Text")
	{
		NewComponent = InSelectedActor->AddComponent<UTextComponent>(NewComponentName);
	}
	else
	{
		UE_LOG_ERROR("ActorDetailWidget: 알 수 없는 컴포넌트 타입 '%s'을(를) 추가할 수 없습니다.", InComponentName.data());
		return;
	}

	if (!NewComponent)
	{
		UE_LOG_ERROR("ActorDetailWidget: '%s' 컴포넌트 생성에 실패했습니다.", InComponentName.data());
		return;
	}

	// 1. 새로 만든 컴포넌트가 SceneComponent인지 확인
	if (USceneComponent* NewSceneComponent = Cast<USceneComponent>(NewComponent))
	{
		// 2. 현재 선택된 컴포넌트가 있고, 그것이 SceneComponent인지 확인
		USceneComponent* ParentSceneComponent = Cast<USceneComponent>(SelectedComponent.Get());

		if (ParentSceneComponent)
		{
			// 3. 선택된 컴포넌트(부모)에 새로 만든 컴포넌트(자식)를 붙임
			//    (SetupAttachment는 UCLASS 내에서 호출하는 것을 가정)
			NewSceneComponent->SetParentAttachment(ParentSceneComponent);
			UE_LOG_SUCCESS("'%s'를 '%s'의 자식으로 추가했습니다.", NewComponentName.ToString().data(), ParentSceneComponent->GetName().ToString().data());
		}
		else
		{
			// 4. 선택된 컴포넌트가 없으면 액터의 루트 컴포넌트에 붙임
			NewSceneComponent->SetParentAttachment(InSelectedActor->GetRootComponent());
			UE_LOG_SUCCESS("'%s'를 액터의 루트에 추가했습니다.", NewComponentName.ToString().data());
		}
	}
	else
	{
		UE_LOG_SUCCESS("Non-Scene Component '%s'를 액터에 추가했습니다.", NewComponentName.ToString().data());
	}
}

void UActorDetailWidget::StartRenamingActor(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	bIsRenamingActor = true;
	FString CurrentName = InActor->GetName().ToString();
	strncpy_s(ActorNameBuffer, CurrentName.data(), sizeof(ActorNameBuffer) - 1);
	ActorNameBuffer[sizeof(ActorNameBuffer) - 1] = '\0';

	UE_LOG("ActorDetailWidget: '%s' 에 대한 이름 변경 시작", CurrentName.data());
}

void UActorDetailWidget::FinishRenamingActor(TObjectPtr<AActor> InActor)
{
	if (!InActor || !bIsRenamingActor)
	{
		return;
	}

	FString NewName = ActorNameBuffer;
	if (!NewName.empty() && NewName != InActor->GetName().ToString())
	{
		// Actor 이름 변경
		InActor->SetName(NewName);
		UE_LOG_SUCCESS("ActorDetailWidget: Actor의 이름을 '%s' (으)로 변경하였습니다", NewName.data());
	}

	bIsRenamingActor = false;
	ActorNameBuffer[0] = '\0';
}

void UActorDetailWidget::CancelRenamingActor()
{
	bIsRenamingActor = false;
	ActorNameBuffer[0] = '\0';
	UE_LOG_WARNING("ActorDetailWidget: 이름 변경 취소");
}

void UActorDetailWidget::RenderTransformEdit()
{
	if (!SelectedComponent)
		return;

	USceneComponent* SceneComponent = Cast<USceneComponent>(SelectedComponent.Get());
	if (!SceneComponent)
		return;

	ImGui::Text("Transform");

	// 컴포넌트 포인터를 PushID로 사용해서 내부 ID 고유화
	ImGui::PushID(SceneComponent);

	FVector ComponentPosition = SceneComponent->GetRelativeLocation();
	if (ImGui::DragFloat3("Position", &ComponentPosition.X, 0.1f))
	{
		SceneComponent->SetRelativeLocation(ComponentPosition);
	}

	FVector ComponentRotation = SceneComponent->GetRelativeRotation();
	if (ImGui::DragFloat3("Rotation", &ComponentRotation.X, 1.0f))
	{
		SceneComponent->SetRelativeRotation(ComponentRotation);
	}

	FVector ComponentScale = SceneComponent->GetRelativeScale3D();
	if (ImGui::DragFloat3("Scale", &ComponentScale.X, 0.1f))
	{
		SceneComponent->SetRelativeScale3D(ComponentScale);
	}

	ImGui::PopID();
}
