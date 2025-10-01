#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/TextComponent.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
	// to do: primitive factory로 빌보드 생성
	BillBoardComponent = new UTextComponent(this, 5.0f);
	OwnedComponents.push_back(TObjectPtr<UTextComponent>(BillBoardComponent));
}

AActor::AActor(UObject* InOuter)
{
	SetOuter(InOuter);
}

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		SafeDelete(Component);
	}
	SetOuter(nullptr);
	OwnedComponents.clear();
}

void AActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (RootComponent)
	{
		RootComponent->Serialize(bInIsLoading, InOutHandle);
	}
}

void AActor::SetActorLocation(const FVector& InLocation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(InLocation);
	}
}

void AActor::SetActorRotation(const FVector& InRotation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeRotation(InRotation);
	}
}

void AActor::SetActorScale3D(const FVector& InScale) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeScale3D(InScale);
	}
}

void AActor::SetUniformScale(bool IsUniform)
{
	if (RootComponent)
	{
		RootComponent->SetUniformScale(IsUniform);
	}
}

bool AActor::IsUniformScale() const
{
	if (RootComponent)
	{
		return RootComponent->IsUniformScale();
	}
	return false;
}

const FVector& AActor::GetActorLocation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeLocation();
}

const FVector& AActor::GetActorRotation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeRotation();
}

const FVector& AActor::GetActorScale3D() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeScale3D();
}

UObject* AActor::Duplicate(UObject* InNewOuter, TMap<UObject*, UObject*>& InOutDuplicationMap)
{
	return Super::Duplicate(InNewOuter, InOutDuplicationMap);
}

void AActor::PostDuplicate(const TMap<UObject*, UObject*>& InDuplicationMap)
{
	// 1. 부모의 PostDuplicate를 먼저 호출하여 기본 참조들을 처리합니다.
	Super::PostDuplicate(InDuplicationMap);

	// 2. 원본 객체를 AActor 타입으로 캐스팅합니다.
	const AActor* OriginalActor = Cast<const AActor>(SourceObject);
	if (!OriginalActor) return;

	// 3. 원본 액터의 RootComponent 포인터를 가져옵니다.
	if (USceneComponent* OriginalRootComponent = OriginalActor->GetRootComponent())
	{
		// 4. DuplicationMap에서 원본 RootComponent에 해당하는 '새로운' RootComponent를 찾습니다.
		if (auto It = InDuplicationMap.find(OriginalRootComponent); It != InDuplicationMap.end())
		{
			// 5. 찾은 새로운 컴포넌트로 나의 RootComponent 포인터를 설정합니다.
			this->RootComponent = TObjectPtr<USceneComponent>(Cast<USceneComponent>(It->second));
		}
	}

	// 6. 내가 소유한 모든 컴포넌트들에게도 PostDuplicate를 전파합니다.
	// (컴포넌트가 다른 객체를 참조할 경우를 대비)
	for (const auto& ComponentPtr : OwnedComponents)
	{
		if (ComponentPtr)
		{
			ComponentPtr->PostDuplicate(InDuplicationMap);
		}
	}
}

void AActor::CopyPropertiesFrom(const UObject* InObject)
{
	// 1. 부모 클래스의 CopyPropertiesFrom을 먼저 호출합니다.
	Super::CopyPropertiesFrom(InObject);

	// 2. AActor 타입으로 안전하게 캐스팅합니다.
	if (const AActor* SourceActor = Cast<const AActor>(InObject))
	{
		// 3. AActor만의 값 타입 멤버들을 복사합니다.
		this->bCanEverTick = SourceActor->bCanEverTick;
	}
}

void AActor::DuplicatesSubObjects(UObject* InNewOuter, TMap<UObject*, UObject*>& InOutDuplicationMap)
{
	// 1. 부모 클래스의 DuplicatesSubObjects를 먼저 호출합니다.
	Super::DuplicatesSubObjects(InNewOuter, InOutDuplicationMap);

	// 2. 원본 객체를 AActor 타입으로 캐스팅합니다.
	const AActor* OriginalActor = Cast<const AActor>(SourceObject);
	if (!OriginalActor) return;

	// 3. 원본 액터의 모든 소유 컴포넌트(OwnedComponents)를 순회하며 복제합니다.
	for (const auto& OriginalComponentPtr : OriginalActor->OwnedComponents)
	{
		if (UActorComponent* OriginalComponent = OriginalComponentPtr.Get())
		{
			// 4. 각 컴포넌트에 대해 재귀적으로 Duplicate 함수를 호출하여 깊은 복사를 수행합니다.
			// 여기서 생성된 새 컴포넌트의 Outer는 '새로운 액터(this)'가 됩니다.
			if (UActorComponent* NewComponent = Cast<UActorComponent>(OriginalComponent->Duplicate(this, InOutDuplicationMap)))
			{
				// 5. 새로 복제된 컴포넌트를 나의 OwnedComponents 목록에 추가합니다.
				this->OwnedComponents.push_back(TObjectPtr<UActorComponent>(NewComponent));
			}
		}
	}
}

void AActor::Tick()
{
	for (auto& Component : OwnedComponents)
	{
		if (Component && Component->CanTick())
		{
			Component->TickComponent();
		}
	}
}

void AActor::BeginPlay()
{
}

void AActor::EndPlay()
{
}
