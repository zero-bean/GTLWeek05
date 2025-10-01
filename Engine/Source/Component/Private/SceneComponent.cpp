#include "pch.h"
#include "Component/Public/SceneComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Level/Public/Level.h"

#include <json.hpp>

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;
}

void USceneComponent::BeginPlay()
{

}

void USceneComponent::TickComponent()
{
	Super::TickComponent();
}

void USceneComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		FJsonSerializer::ReadVector(InOutHandle, "Location", RelativeLocation, FVector::ZeroVector());
		FJsonSerializer::ReadVector(InOutHandle, "Rotation", RelativeRotation, FVector::ZeroVector());
		FJsonSerializer::ReadVector(InOutHandle, "Scale", RelativeScale3D, FVector::OneVector());
		MarkAsDirty();
	}
	// 저장
	else
	{
		InOutHandle["Location"] = FJsonSerializer::VectorToJson(RelativeLocation);
		InOutHandle["Rotation"] = FJsonSerializer::VectorToJson(RelativeRotation);
		InOutHandle["Scale"] = FJsonSerializer::VectorToJson(RelativeScale3D);
	}
}

void USceneComponent::SetParentAttachment(USceneComponent* NewParent)
{
	if (NewParent == this)
	{
		return;
	}

	if (NewParent == ParentAttachment)
	{
		return;
	}

	//부모의 조상중에 내 자식이 있으면 순환참조 -> 스택오버플로우 일어남.
	for (USceneComponent* Ancester = NewParent; NewParent; Ancester = NewParent->ParentAttachment)
	{
		if (NewParent == this) //조상중에 내 자식이 있다면 조상중에 내가 있을 것임.
			return;
	}

	//부모가 될 자격이 있음, 이제 부모를 바꿈.

	if (ParentAttachment) //부모 있었으면 이제 그 부모의 자식이 아님
	{
		ParentAttachment->RemoveChild(this);
	}

	ParentAttachment = NewParent;

	MarkAsDirty();

}

void USceneComponent::RemoveChild(USceneComponent* ChildDeleted)
{
	Children.erase(std::remove(Children.begin(), Children.end(), this), Children.end());
}

void USceneComponent::CopyPropertiesFrom(const UObject* InObject)
{
	// 1. 부모(UActorComponent)의 작업을 먼저 호출합니다.
	Super::CopyPropertiesFrom(InObject);

	// 2. 원본을 USceneComponent로 캐스팅합니다.
	if (const USceneComponent* SourceComponent = Cast<const USceneComponent>(InObject))
	{
		// 3. USceneComponent의 고유한 '값' 타입 멤버들을 복사합니다.
		// (트랜스폼 정보)
		this->RelativeLocation = SourceComponent->RelativeLocation;
		this->RelativeRotation = SourceComponent->RelativeRotation;
		this->RelativeScale3D = SourceComponent->RelativeScale3D;
		this->bIsUniformScale = SourceComponent->bIsUniformScale;

		// 중요: ParentAttachment, Children 같은 포인터/컨테이너는 여기서 복사하지 않습니다.
	}
}

void USceneComponent::MarkAsDirty()
{
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;

	for (USceneComponent* Child : Children)
	{
		Child->MarkAsDirty();
	}
}

void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	RelativeLocation = Location;
	MarkAsDirty();

	if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(this))
	{
		GWorld->GetLevel()->UpdatePrimitiveInOctree(PrimitiveComponent);
	}
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	RelativeRotation = Rotation;
	MarkAsDirty();

	if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(this))
	{
		GWorld->GetLevel()->UpdatePrimitiveInOctree(PrimitiveComponent);
	}
}

void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	RelativeScale3D = Scale;
	MarkAsDirty();

	if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(this))
	{
		GWorld->GetLevel()->UpdatePrimitiveInOctree(PrimitiveComponent);
	}
}

void USceneComponent::PostDuplicate(const TMap<UObject*, UObject*>& InDuplicationMap)
{
	// 1. 부모(UActorComponent)의 작업을 먼저 호출하여 Owner 포인터를 설정합니다.
	Super::PostDuplicate(InDuplicationMap);

	// 2. 원본 컴포넌트를 가져옵니다.
	const USceneComponent* OriginalComponent = Cast<const USceneComponent>(SourceObject);
	if (!OriginalComponent) return;

	// 3. 원본의 부모(AttachParent)를 가져옵니다.
	USceneComponent* OriginalParent = OriginalComponent->ParentAttachment;
	if (OriginalParent)
	{
		// 4. DuplicationMap에서 원본 부모에 해당하는 '새로운 부모'를 찾습니다.
		auto It = InDuplicationMap.find(OriginalParent);
		if (It != InDuplicationMap.end())
		{
			// 5. 찾은 새로운 부모 컴포넌트를 나의 부모로 설정합니다.
			// (내부적으로 Children 목록에도 추가됩니다)
			USceneComponent* NewParent = Cast<USceneComponent>(It->second);
			this->SetParentAttachment(NewParent);
		}
	}
}