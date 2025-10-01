#pragma once
#include "Core/Public/Object.h"
#include "Core/Public/ObjectPtr.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Factory/Public/NewObject.h"

class UUUIDTextComponent;
/**
 * @brief Level에서 렌더링되는 UObject 클래스
 * UWorld로부터 업데이트 함수가 호출되면 component들을 순회하며 위치, 애니메이션, 상태 처리
 */
UCLASS()

class AActor : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(AActor, UObject)

public:
	AActor();
	AActor(UObject* InOuter);
	~AActor() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void SetActorLocation(const FVector& InLocation) const;
	void SetActorRotation(const FVector& InRotation) const;
	void SetActorScale3D(const FVector& InScale) const;
	void SetUniformScale(bool IsUniform);

	bool IsUniformScale() const;

	virtual void BeginPlay();
	virtual void EndPlay();
	virtual void Tick();

	// Duplication
	virtual UObject* Duplicate(UObject* InNewOuter, TMap<UObject*, UObject*>& InOutDuplicationMap) override;
	virtual void PostDuplicate(const TMap<UObject*, UObject*>& InDuplicationMap) override;


	// Getter & Setter
	USceneComponent* GetRootComponent() const { return RootComponent.Get(); }
	const TArray<TObjectPtr<UActorComponent>>& GetOwnedComponents() const { return OwnedComponents; }

	void SetRootComponent(USceneComponent* InOwnedComponents) { RootComponent = InOwnedComponents; }

	UUUIDTextComponent* GetUUIDTextComponent() const { return UUIDTextComponent.Get(); }

	const FVector& GetActorLocation() const;
	const FVector& GetActorRotation() const;
	const FVector& GetActorScale3D() const;

	template<class T>
	T* CreateDefaultSubobject(const FName& InName)
	{
		static_assert(is_base_of_v<UObject, T>, "생성할 클래스는 UObject를 반드시 상속 받아야 합니다");

		// 1. 템플릿 타입 T로부터 UClass 정보를 가져옵니다.
		TObjectPtr<UClass> ComponentClass = T::StaticClass();

		// 2. NewObject를 호출할 때도 템플릿 타입 T를 사용하여 정확한 타입의 컴포넌트를 생성합니다.
		TObjectPtr<T> NewComponent = NewObject<T>(TObjectPtr<UObject>(this), ComponentClass, InName);

		// 3. 컴포넌트 생성이 성공했는지 확인하고 기본 설정을 합니다.
		if (NewComponent)
		{
			NewComponent->SetOwner(this);
			OwnedComponents.push_back(NewComponent);
		}

		// 4. 정확한 타입(T*)으로 캐스팅 없이 바로 반환합니다.
		return NewComponent;
	}

	/**
	 * @brief 런타임에 이 액터에 새로운 컴포넌트를 생성하고 등록합니다.
	 * @tparam T UActorComponent를 상속받는 컴포넌트 타입
	 * @param InName 컴포넌트의 이름
	 * @return 생성된 컴포넌트를 가리키는 포인터
	 */
	template<class T>
	T* AddComponent(const FName& InName)
	{
		static_assert(std::is_base_of_v<UActorComponent, T>, "추가할 클래스는 UActorComponent를 반드시 상속 받아야 합니다");

		// 1. NewObject는 여전히 로우 포인터(T*)를 반환합니다.
		T* NewComponent = NewObject<T>(this, T::StaticClass(), InName);

		if (NewComponent)
		{
			// 2. 로우 포인터를 RegisterComponent에 전달합니다.
			//    T* -> TObjectPtr<UActorComponent> 로의 안전한 암시적 변환이 일어납니다.
			RegisterComponent(NewComponent);
		}

		return NewComponent;
	}

	void RegisterComponent(TObjectPtr<UActorComponent> InNewComponent);

	bool CanTick() const { return bCanEverTick; }
	void SetCanTick(bool InbCanEverTick) { bCanEverTick = InbCanEverTick; }

	bool CanTickInEditor() const { return bTickInEditor; }
	void SetTickInEditor(bool InbTickInEditor) { bTickInEditor = InbTickInEditor; }

protected:
	// Duplication
	virtual void CopyPropertiesFrom(const UObject* InObject) override;
	virtual void DuplicatesSubObjects(UObject* InNewOuter, TMap<UObject*, UObject*>& InOutDuplicationMap) override;

	bool bCanEverTick = false;
	bool bTickInEditor = false;
	bool bBegunPlay = false;

private:
	TObjectPtr<USceneComponent> RootComponent = nullptr;
	TObjectPtr<UUUIDTextComponent> UUIDTextComponent = nullptr;
	TArray<TObjectPtr<UActorComponent>> OwnedComponents;
};
