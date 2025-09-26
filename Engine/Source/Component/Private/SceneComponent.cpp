#include "pch.h"
#include "Component/Public/SceneComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"

#include <json.hpp>

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;
}

void USceneComponent::BeginPlay()
{
	PreviousRelativeLocation = RelativeLocation;
	PreviousRelativeRotation = RelativeRotation;
	PreviousRelativeScale3D = RelativeScale3D;
}

void USceneComponent::TickComponent()
{
	Super::TickComponent();

	// 한번이라도 움직인 경우 Mobility를 Dynamic으로 설정하고 더 이상 Static으로 되돌리지 않습니다.
	if (Mobility == EComponentMobility::Static)
	{
		if (PreviousRelativeLocation != RelativeLocation ||
			PreviousRelativeRotation != RelativeRotation ||
			PreviousRelativeScale3D != RelativeScale3D)
		{
			Mobility = EComponentMobility::Dynamic;
		}
	}

	// 4. 다음 프레임에서 비교할 수 있도록 현재 상태를 "이전 상태"로 저장
	PreviousRelativeLocation = RelativeLocation;
	PreviousRelativeRotation = RelativeRotation;
	PreviousRelativeScale3D = RelativeScale3D;
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

void USceneComponent::MarkAsDirty()
{
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;

	for (USceneComponent* Child : Children)
	{
		Child->MarkAsDirty();
	}
}

