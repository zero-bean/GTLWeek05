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
	virtual UClass* GetDefaultRootComponent();
	virtual void InitializeComponents();

	bool IsUniformScale() const;

	virtual void BeginPlay();
	virtual void EndPlay();
	virtual void Tick(float DeltaTimes);

	// Getter & Setter
	USceneComponent* GetRootComponent() const { return RootComponent.Get(); }
	TArray<TObjectPtr<UActorComponent>>& GetOwnedComponents()  { return OwnedComponents; }

	void SetRootComponent(USceneComponent* InOwnedComponents) { RootComponent = InOwnedComponents; }

	UUUIDTextComponent* GetUUIDTextComponent() const { return UUIDTextComponent.Get(); }
	void SetUUIDTextComponent(UUUIDTextComponent* InUUIDTextComponent) { UUIDTextComponent = InUUIDTextComponent; }

	const FVector& GetActorLocation() const;
	const FVector& GetActorRotation() const;
	const FVector& GetActorScale3D() const;

	template<class T>
	T* CreateDefaultSubobject(const FName& InName = FName::None)
	{
		static_assert(is_base_of_v<UObject, T>, "생성할 클래스는 UObject를 반드시 상속 받아야 합니다");

		// 2. NewObject를 호출할 때도 템플릿 타입 T를 사용하여 정확한 타입의 컴포넌트를 생성합니다.
		TObjectPtr<T> NewComponent = NewObject<T>(TObjectPtr<UObject>(this));
		if (!InName.IsNone()) { NewComponent->SetName(InName); }

		// 3. 컴포넌트 생성이 성공했는지 확인하고 기본 설정을 합니다.
		if (NewComponent)
		{
			NewComponent->SetOwner(this);
			OwnedComponents.push_back(NewComponent);
		}

		// 4. 정확한 타입(T*)으로 캐스팅 없이 바로 반환합니다.
		return NewComponent;
	}
	
	UActorComponent* CreateDefaultSubobject(UClass* Class)
	{
		UActorComponent* NewComponent = Cast<UActorComponent>(::NewObject(Class, TObjectPtr<UObject>(this)));
		if (NewComponent)
		{
			NewComponent->SetOwner(this);
			OwnedComponents.push_back(NewComponent);
		}
		
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

		// 1. 정의하신 NewObject<T>() 함수를 호출합니다.
		//    이 함수는 TObjectPtr을 반환하므로 .Get()으로 원시 포인터를 가져옵니다.
		T* NewComponent = NewObject<T>(this).Get();

		if (NewComponent)
		{
			// 3. 컴포넌트를 액터에 등록합니다.
			RegisterComponent(NewComponent);
		}

		return NewComponent;
	}

	bool RemoveComponent(UActorComponent* Target)
	{
		for (auto Itr = OwnedComponents.begin(); Itr != OwnedComponents.end(); Itr++)
		{
			if (*Itr == Target)
			{
				OwnedComponents.erase(Itr);
				return true;
			}
		}
		return false;
	}

	void RegisterComponent(TObjectPtr<UActorComponent> InNewComponent);

	bool CanTick() const { return bCanEverTick; }
	void SetCanTick(bool InbCanEverTick) { bCanEverTick = InbCanEverTick; }

	bool CanTickInEditor() const { return bTickInEditor; }
	void SetTickInEditor(bool InbTickInEditor) { bTickInEditor = InbTickInEditor; }

protected:
	bool bCanEverTick = false;
	bool bTickInEditor = false;
	bool bBegunPlay = false;

private:
	TObjectPtr<USceneComponent> RootComponent = nullptr;
	TObjectPtr<UUUIDTextComponent> UUIDTextComponent = nullptr;
	TArray<TObjectPtr<UActorComponent>> OwnedComponents;
	
public:
	virtual UObject* Duplicate() override;

	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;
};
