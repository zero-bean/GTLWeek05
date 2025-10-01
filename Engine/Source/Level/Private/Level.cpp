#include "pch.h"
#include "Level/Public/Level.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/CubeActor.h"
#include "Actor/Public/SphereActor.h"
#include "Actor/Public/TriangleActor.h"
#include "Actor/Public/SquareActor.h"
#include "Factory/Public/NewObject.h"
#include "Factory/Public/FactorySystem.h"
#include "Core/Public/Object.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Viewport.h"
#include "Utility/Public/JsonSerializer.h"
#include "Utility/Public/ActorTypeMapper.h"
#include "Global/Octree.h"
#include <json.hpp>

// World에서 이뤄져야 하는데, 현재는 클래스가 없으니 임시로 레벨에 작성 [2025/10/01, 박영빈]
namespace
{
	// 최상위 복제 관리 함수
	UObject* DuplicateObjectGraph(UObject* InObjectToDuplicate, UObject* InNewOuter)
	{
		if (!InObjectToDuplicate) return nullptr;

		TMap<UObject*, UObject*> DuplicationMap;

		// Pass 1: 객체 생성 및 복사
		UObject* DuplicatedRootObject = InObjectToDuplicate->Duplicate(InNewOuter, DuplicationMap);

		// Pass 2: 참조 재연결
		for (auto const& [Original, Duplicated] : DuplicationMap)
		{
			if (Duplicated)
			{
				Duplicated->PostDuplicate(DuplicationMap);
			}
		}

		return DuplicatedRootObject;
	}
}

// World에서 이뤄져야 하는데, 현재는 클래스가 없으니 임시로 레벨에 작성 [2025/10/01, 박영빈]
AActor* ULevel::DuplicateActor(AActor* InActorToDuplicate)
{
	if (!InActorToDuplicate)
	{
		return nullptr;
	}

	// 1. 자신과 일치하는 레플리카를 생성합니다.
	if (AActor* NewActor = Cast<AActor>(DuplicateObjectGraph(InActorToDuplicate, this)))
	{
		// 2. 복제된 액터를 레벨의 관리 목록에 추가합니다.
		LevelActors.push_back(TObjectPtr(NewActor));

		// 4. 액터의 Primitive 컴포넌트들을 옥트리에 등록합니다.
		AddLevelPrimitiveComponent(NewActor);

		// (선택) 복제된 액터를 즉시 선택 상태로 만들 수 있습니다.
		SetSelectedActor(NewActor);

		return NewActor;
	}

	return nullptr;
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
			PrimitiveJson["Type"] = FActorTypeMapper::ActorToType(Actor->GetClass());;
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

void ULevel::Update()
{
	// Process Delayed Task
	ProcessPendingDeletions();

	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			if (Actor->CanTick()) { Actor->Tick(); }
		}
	}

	/* * 
	* 해당 코드는 불법 증축을 진행한 코드입니다. 이 코드를 정상화하는 방법은 다음과 같습니다.
	* 1. 모든 액터 및 컴포넌트의 bEverTick 관련 기능을 싹 걷어야 합니다.
	* 2. Scencomponent의 TickComponent에 시간 체크 기능을 넣으면 됩니다.
	* 성능 저하로 인해 Week05 한정으로 만든 코드이니 잘 처리하시길 바랍니다. [PYB]
	*/
	for (UPrimitiveComponent* Primitive : DynamicPrimitives)
	{
		if (Primitive == nullptr) { continue; }

		Primitive->InactivityTimer += UTimeManager::GetInstance().GetDeltaTime();
		if (Primitive->InactivityTimer >= Primitive->InactivityThreshold)
		{
			if (StaticOctree->Insert(Primitive))
			{
				if (auto It = std::find(DynamicPrimitives.begin(), DynamicPrimitives.end(), Primitive); It != DynamicPrimitives.end())
				{
					*It = std::move(DynamicPrimitives.back());
					DynamicPrimitives.pop_back();
				}
				Primitive->InactivityTimer = 0.f;
			}
		}
	}
}

void ULevel::Tick(float DeltaTime)
{
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
	SetSelectedActor(nullptr);

	// 1. 지연 삭제 목록에 남아있는 액터들을 먼저 처리합니다.
	ProcessPendingDeletions();

	// 2. LevelActors 배열에 남아있는 모든 액터의 메모리를 해제합니다.
	for (const auto& Actor : LevelActors)
	{
		delete Actor;
	}
	LevelActors.clear();

	// 3. 모든 액터 객체가 삭제되었으므로, 포인터를 담고 있던 컨테이너들을 비웁니다.
	SafeDelete(StaticOctree);
	DynamicPrimitives.clear();
	ActorsToDelete.clear();

	// 4. 선택된 액터 참조를 안전하게 해제합니다.
	SelectedActor = nullptr;
}

AActor* ULevel::SpawnActorToLevel(UClass* InActorClass, const FName& InName, JSON* ActorJsonData)
{
	if (!InActorClass)
	{
		return nullptr;
	}

	AActor* NewActor = NewObject<AActor>(nullptr, TObjectPtr(InActorClass), InName);
	if (NewActor)
	{
		if (InName != FName::GetNone())
		{
			NewActor->SetName(InName);
		}
		if (ActorJsonData != nullptr)
		{
			NewActor->Serialize(true, *ActorJsonData);
		}
		LevelActors.push_back(TObjectPtr(NewActor));
		NewActor->BeginPlay();
		AddLevelPrimitiveComponent(NewActor);
		return NewActor;
	}

	return nullptr;
}

void ULevel::RequestToUpdateLeveInfo(UPrimitiveComponent* InPrimitive)
{
}

void ULevel::AddLevelPrimitiveComponent(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
		if (!PrimitiveComponent) { continue; }

		if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::BillBoard) { continue; }

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

		return true;
	}


	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}

	// Remove
	delete InActor;

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}

// 지연 삭제를 위한 마킹
void ULevel::MarkActorForDeletion(AActor* InActor)
{
	if (!InActor)
	{
		UE_LOG("Level: MarkActorForDeletion: InActor Is Null");
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("Level: Actor Already Marked For Deletion");
			return;
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	UE_LOG("Level: 다음 Tick에 Actor를 제거하기 위한 마킹 처리: %s", InActor->GetName().ToString().data());

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}
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
