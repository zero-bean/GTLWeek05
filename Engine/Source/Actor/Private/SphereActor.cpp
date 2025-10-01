#include "pch.h"
#include "Actor/Public/SphereActor.h"
#include "Component/Mesh/Public/SphereComponent.h"

IMPLEMENT_CLASS(ASphereActor, AActor)

ASphereActor::ASphereActor()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SphereComponent->SetParentAttachment(GetRootComponent());
}

void ASphereActor::PostDuplicate(const TMap<UObject*, UObject*>& InDuplicationMap)
{
	// 1. 부모 클래스(AActor)의 PostDuplicate를 먼저 호출합니다.
	// 이 호출로 RootComponent 포인터와 기본 참조들이 모두 처리됩니다.
	Super::PostDuplicate(InDuplicationMap);

	// 2. 원본 객체에서 원래 SphereComponent 포인터를 찾습니다.
	if (const ASphereActor* OriginalActor = Cast<const ASphereActor>(SourceObject))
	{
		// 3. DuplicationMap을 사용해 원본 컴포넌트에 해당하는 '새로운' 컴포넌트를 찾습니다.
		if (auto It = InDuplicationMap.find(OriginalActor->SphereComponent); It != InDuplicationMap.end())
		{
			// 4. 찾은 새로운 컴포넌트로 나의 SphereComponent 포인터를 설정합니다.
			this->SphereComponent = Cast<USphereComponent>(It->second);
		}
	}
}
