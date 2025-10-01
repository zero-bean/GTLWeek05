#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/UUIDTextComponent.h"
#include "Level/Public/Level.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
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

UClass* AActor::GetDefaultRootComponent()
{
	return USceneComponent::StaticClass();
}

void AActor::InitializeComponents()
{
	USceneComponent* SceneComp = Cast<USceneComponent>(CreateDefaultSubobject(GetDefaultRootComponent()));
	SetRootComponent(SceneComp);

	UUIDTextComponent = CreateDefaultSubobject<UUUIDTextComponent>();
	UUIDTextComponent->SetParentAttachment(GetRootComponent());
	UUIDTextComponent->SetOffset(5.0f);
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

UObject* AActor::Duplicate()
{
	AActor* Actor = Cast<AActor>(Super::Duplicate());
	Actor->bCanEverTick = bCanEverTick;
	return Actor;
}

void AActor::RegisterComponent(TObjectPtr<UActorComponent> InNewComponent)
{
	if (!InNewComponent || InNewComponent->GetOwner() != this)
	{
		InNewComponent->SetOwner(this);
	}

	OwnedComponents.push_back(InNewComponent);

	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(InNewComponent.Get()))
	{
		GWorld->GetLevel()->RegisterPrimitiveComponent(PrimitiveComponent);
	}
}

void AActor::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	AActor* DuplicatedActor = Cast<AActor>(DuplicatedObject);
	USceneComponent* DuplicatedRoot = Cast<USceneComponent>(GetRootComponent()->Duplicate());
	DuplicatedRoot->SetOwner(DuplicatedActor);
	DuplicatedActor->SetRootComponent(DuplicatedRoot);
	DuplicatedActor->OwnedComponents.push_back(DuplicatedRoot);
	
	for (UActorComponent* Component : OwnedComponents)
	{
		if (!Cast<USceneComponent>(Component))
		{
			UActorComponent* DuplicatedActorComponent = Cast<UActorComponent>(Component->Duplicate());
			DuplicatedActor->OwnedComponents.push_back(DuplicatedActorComponent);
			DuplicatedActorComponent->SetOwner(DuplicatedActor);
		}
	}
}

void AActor::Tick(float DeltaTimes)
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
	if (bBegunPlay) return;
	bBegunPlay = true;
	for (auto& Component : OwnedComponents)
	{
		if (Component)
		{
			Component->BeginPlay();
		}
	}
}

void AActor::EndPlay()
{
	if (!bBegunPlay) return;
	bBegunPlay = false;
	for (auto& Component : OwnedComponents)
	{
		if (Component)
		{
			Component->EndPlay();
		}
	}
}
