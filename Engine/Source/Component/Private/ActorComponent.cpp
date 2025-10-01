#include "pch.h"
#include "Component/Public/ActorComponent.h"

IMPLEMENT_CLASS(UActorComponent, UObject)

UActorComponent::UActorComponent()
{
	ComponentType = EComponentType::Actor;
}

UActorComponent::~UActorComponent()
{
	SetOuter(nullptr);
}

void UActorComponent::BeginPlay()
{

}

void UActorComponent::TickComponent()
{

}

void UActorComponent::EndPlay()
{

}


void UActorComponent::OnSelected()
{

}


void UActorComponent::OnDeselected()
{

}

void UActorComponent::CopyPropertiesFrom(const UObject* InObject)
{
	// 1. 부모(UObject)의 작업을 먼저 호출합니다.
	Super::CopyPropertiesFrom(InObject);

	// 2. 원본을 UActorComponent로 캐스팅합니다.
	if (const UActorComponent* SourceComponent = Cast<const UActorComponent>(InObject))
	{
		// 3. UActorComponent의 고유한 '값' 타입 멤버를 복사합니다.
		this->bCanEverTick = SourceComponent->bCanEverTick;
		this->ComponentType = SourceComponent->ComponentType;

		// 중요: Owner 같은 포인터는 여기서 복사하면 안 됩니다!
		// 얕은 복사가 되어 원본 액터를 가리키게 되기 때문입니다.
	}
}

void UActorComponent::PostDuplicate(const TMap<UObject*, UObject*>& InDuplicationMap)
{
	// 1. 부모(UObject)의 작업을 먼저 호출합니다.
	Super::PostDuplicate(InDuplicationMap);

	// 2. 복제된 컴포넌트의 새로운 주인(Owner)을 설정합니다.
	// UObject::Duplicate 과정에서 새로운 Outer(주인 액터)가 설정되었으므로,
	// GetOuter()를 통해 가져와 Owner 포인터에 연결해 줍니다.
	this->Owner = Cast<AActor>(GetOuter());
}
