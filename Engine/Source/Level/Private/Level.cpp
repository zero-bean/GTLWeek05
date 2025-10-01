#include "pch.h"
#include "Level/Public/Level.h"
#include "Component/Public/PrimitiveComponent.h"

#include "Manager/UI/Public/UIManager.h"
#include "Actor/Public/Actor.h"
#include "Factory/Public/NewObject.h"
#include "Core/Public/Object.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Viewport.h"
#include "Utility/Public/JsonSerializer.h"
#include "Utility/Public/ActorTypeMapper.h"
#include "Global/Octree.h"
#include <json.hpp>

IMPLEMENT_CLASS(ULevel, UObject)

ULevel::ULevel()
{
	StaticOctree = new FOctree(FVector(0, 0, -5), 75, 0);
}

ULevel::ULevel(const FName& InName)
	: UObject(InName)
{
	StaticOctree = new FOctree(FVector(0, 0, -5), 75, 0);
}

ULevel::~ULevel()
{
	// 소멸자는 Cleanup 함수를 호출하여 모든 리소스를 정리하도록 합니다.
	Cleanup();
}

void ULevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		// NOTE: Version 사용하지 않음
		//uint32 Version = 0;
		//FJsonSerializer::ReadUint32(InOutHandle, "Version", Version);

		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		uint32 NextUUID = 0;
		FJsonSerializer::ReadUint32(InOutHandle, "NextUUID", NextUUID);

		JSON PerspectiveCameraData;
		if (FJsonSerializer::ReadObject(InOutHandle, "PerspectiveCamera", PerspectiveCameraData))
		{
			UConfigManager::GetInstance().SetCameraSettingsFromJson(PerspectiveCameraData);
			URenderer::GetInstance().GetViewportClient()->ApplyAllCameraDataToViewportClients();
		}

		JSON PrimitivesJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "Primitives", PrimitivesJson))
		{
			// ObjectRange()를 사용하여 Primitives 객체의 모든 키-값 쌍을 순회
			for (auto& Pair : PrimitivesJson.ObjectRange())
			{
				// Pair.first는 ID 문자열, Pair.second는 단일 프리미티브의 JSON 데이터입니다.
				const FString& IdString = Pair.first;
				JSON& PrimitiveDataJson = Pair.second;

				FString TypeString;
				FJsonSerializer::ReadString(PrimitiveDataJson, "Type", TypeString);

				UClass* NewClass = FActorTypeMapper::TypeToActor(TypeString);

				AActor* NewActor = SpawnActorToLevel(NewClass, IdString, &PrimitiveDataJson);
			}
		}
	}

	// 저장
	else
	{
		// NOTE: Version 사용하지 않음
		//InOutHandle["Version"] = 1;

		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		InOutHandle["NextUUID"] = 0;

		// GetCameraSetting 호출 전에 뷰포트 클라이언트의 최신 데이터를 ConfigManager로 동기화합니다.
		URenderer::GetInstance().GetViewportClient()->UpdateCameraSettingsToConfig();
		InOutHandle["PerspectiveCamera"] = UConfigManager::GetInstance().GetCameraSettingsAsJson();

		JSON PrimitivesJson = json::Object();
		for (const TObjectPtr<AActor>& Actor : LevelActors)
		{
			JSON PrimitiveJson;
			PrimitiveJson["Type"] = FActorTypeMapper::ActorToType(Actor->GetClass());
			Actor->Serialize(bInIsLoading, PrimitiveJson);

			PrimitivesJson[std::to_string(Actor->GetUUID())] = PrimitiveJson;
		}
		InOutHandle["Primitives"] = PrimitivesJson;
	}
}

void ULevel::Init()
{
	// TEST CODE
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
	SetSelectedActor(nullptr);

	// LevelActors 배열에 남아있는 모든 액터의 메모리를 해제합니다.
	for (const auto& Actor : LevelActors)
	{
		delete Actor;
	}
	LevelActors.clear();

	// 모든 액터 객체가 삭제되었으므로, 포인터를 담고 있던 컨테이너들을 비웁니다.
	SafeDelete(StaticOctree);
	DynamicPrimitives.clear();
	ActorsToDelete.clear();

	// 선택된 액터 참조를 안전하게 해제합니다.
	SelectedActor = nullptr;
}

AActor* ULevel::SpawnActorToLevel(UClass* InActorClass, const FName& InName, JSON* ActorJsonData)
{
	if (!InActorClass)
	{
		return nullptr;
	}

	AActor* NewActor = Cast<AActor>(NewObject(InActorClass));
	if (NewActor)
	{
		if (!InName.IsNone())
		{
			NewActor->SetName(InName);
		}
		LevelActors.push_back(TObjectPtr(NewActor));
		NewActor->InitializeComponents();
		if (ActorJsonData != nullptr)
		{
			NewActor->Serialize(true, *ActorJsonData);
		}
		NewActor->BeginPlay();
		AddLevelPrimitiveComponent(NewActor);
		return NewActor;
	}

	return nullptr;
}

void ULevel::RegisterPrimitiveComponent(UPrimitiveComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	// StaticOctree에 먼저 삽입 시도
	if (StaticOctree->Insert(InComponent) == false)
	{
		// 실패하면 DynamicPrimitives 목록에 추가
		// 중복 추가를 방지하기 위해 이미 있는지 확인
		if (std::find(DynamicPrimitives.begin(), DynamicPrimitives.end(), InComponent) == DynamicPrimitives.end())
		{
			DynamicPrimitives.push_back(InComponent);
		}
	}

	UE_LOG("Level: '%s' 컴포넌트를 씬에 등록했습니다.", InComponent->GetName().ToString().data());
}

void ULevel::AddLevelPrimitiveComponent(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
		if (!PrimitiveComponent) { continue; }

		if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::Text) { continue; }

		if (StaticOctree->Insert(PrimitiveComponent) == false)
		{
			DynamicPrimitives.push_back(PrimitiveComponent);
		}
	}
}

void ULevel::SetSelectedActor(AActor* InActor)
{
	// Set Selected Actor
	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			Component->OnDeselected();
		}
	}

	if (InActor != SelectedActor)
	{
		UUIManager::GetInstance().OnSelectedActorChanged(InActor);
	}
	SelectedActor = InActor;

	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			Component->OnSelected();
		}
	}
}

// Level에서 Actor 제거하는 함수
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor) return false;

	// 컴포넌트들을 옥트리에서 제거
	for (auto& Component : InActor->GetOwnedComponents())
	{
		TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
		if (!PrimitiveComponent) { continue; }

		if (StaticOctree)
		{
			if (StaticOctree->Remove(PrimitiveComponent) == false)
			{
				if (auto It = std::find(DynamicPrimitives.begin(), DynamicPrimitives.end(), PrimitiveComponent); It != DynamicPrimitives.end())
				{
					*It = std::move(DynamicPrimitives.back());
					DynamicPrimitives.pop_back();
				}
			}
		}
	}

	// LevelActors 리스트에서 제거
	if (auto It = std::find(LevelActors.begin(), LevelActors.end(), InActor); It != LevelActors.end())
	{
		*It = std::move(LevelActors.back());
		LevelActors.pop_back();
	}


	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SetSelectedActor(nullptr);
	}

	// Remove
	SafeDelete(InActor);

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}

void ULevel::UpdatePrimitiveInOctree(UPrimitiveComponent* Primitive)
{
	if (!Primitive) { return; }

	// 1. StaticOctree에서 제거 먼저 시도
	if (StaticOctree->Remove(Primitive))
	{
		// 2. DynamicPrimitives로 등록
		DynamicPrimitives.push_back(Primitive);
	}
	else
	{
		// 3. Static에 없으면 그냥 Dynamic에 넣어줌 (중복 방지 체크 필요)
		if (std::find(DynamicPrimitives.begin(), DynamicPrimitives.end(), Primitive) == DynamicPrimitives.end())
		{
			DynamicPrimitives.push_back(Primitive);
		}
	}
}

void ULevel::ProcessPendingDeletions()
{
	if (ActorsToDelete.empty())
	{
		return;
	}

	UE_LOG("Level: %zu개의 객체 지연 삭제 프로세스 처리 시작", ActorsToDelete.size());

	// 원본 배열을 복사하여 사용 (DestroyActor가 원본을 수정할 가능성에 대비)
	TArray<AActor*> ActorsToProcess = ActorsToDelete;
	ActorsToDelete.clear();

	for (AActor* ActorToDelete : ActorsToProcess)
	{
		if (ActorToDelete)
		{
			DestroyActor(ActorToDelete);
		}
	}

	UE_LOG("Level: 모든 지연 삭제 프로세스 완료");
}

UObject* ULevel::Duplicate()
{
	ULevel* Level = Cast<ULevel>(Super::Duplicate());
	Level->ShowFlags = ShowFlags;
	return Level;
}

void ULevel::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	ULevel* DuplicatedLevel = Cast<ULevel>(DuplicatedObject);

	for (AActor* Actor : LevelActors)
	{
		AActor* DuplicatedActor = Cast<AActor>(Actor->Duplicate());
		DuplicatedLevel->LevelActors.push_back(DuplicatedActor);
		DuplicatedLevel->AddLevelPrimitiveComponent(DuplicatedActor);
	}
}
