#pragma once
#include "Core/Public/Object.h"

class AActor;
class UWidget;

UCLASS()
class UActorComponent : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UActorComponent, UObject)

public:
	UActorComponent();
	~UActorComponent();
	/*virtual void Render(const URenderer& Renderer) const
	{

	}*/

	virtual void BeginPlay();
	virtual void TickComponent();
	virtual void EndPlay();

	virtual void OnSelected();
	virtual void OnDeselected();

	/**
	 * @brief 특정 컴포넌트 전용 Widget이 필요할 경우 재정의 필요
	 */
	virtual TObjectPtr<UClass> GetSpecificWidgetClass() const { return nullptr; }

	virtual void PostDuplicate(const TMap<UObject*, UObject*>& InDuplicationMap) override;

	EComponentType GetComponentType() { return ComponentType; }

	void SetOwner(AActor* InOwner) { Owner = InOwner; }
	AActor* GetOwner() const { return Owner; }

	EComponentType GetComponentType() const { return ComponentType; }

	bool CanTick() const { return bCanEverTick; }
	void SetCanTick(bool InbCanEverTick) { bCanEverTick = InbCanEverTick; }

protected:
	virtual void CopyPropertiesFrom(const UObject* InObject) override;

	EComponentType ComponentType;
	bool bCanEverTick = false;

private:
	AActor* Owner;
};
