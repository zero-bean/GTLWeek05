#include "pch.h"
#include "Core/Public/Object.h"
#include "Core/Public/EngineStatics.h"
#include "Core/Public/Name.h"

#include <json.hpp>

uint32 UEngineStatics::NextUUID = 0;

TArray<TObjectPtr<UObject>>& GetUObjectArray()
{
	static TArray<TObjectPtr<UObject>> GUObjectArray;
	return GUObjectArray;
}

IMPLEMENT_CLASS_BASE(UObject)

UObject::UObject()
	: Name(FName::GetNone()), Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();
	Name = FName("Object_" + to_string(UUID));

	GetUObjectArray().emplace_back(this);
	InternalIndex = static_cast<uint32>(GetUObjectArray().size()) - 1;
}

UObject::UObject(const FName& InName)
	: Name(InName)
	, Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();

	GetUObjectArray().emplace_back(this);
	InternalIndex = static_cast<uint32>(GetUObjectArray().size()) - 1;
}

UObject::~UObject()
{
	/** @todo: 이후에 리뷰 필요 */

	// std::vector에 맞는 올바른 인덱스 유효성 검사
	if (InternalIndex < GetUObjectArray().size())
	{
		GetUObjectArray()[InternalIndex] = nullptr;
	}
}

void UObject::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
}

UObject* UObject::Duplicate(UObject* InNewOuter, TMap<UObject*, UObject*>& InOutDuplicationMap)
{
	// 0. 이미  복제된 객체가 있다는 뜻이므로, 반환 및 종료
	if (auto It = InOutDuplicationMap.find(this); It != InOutDuplicationMap.end()) { return It->second; }

	// 1. 깊은 복사를 통해 텅 빈 객체를 생성
	UObject* NewObject = GetClass()->CreateDefaultObject();
	InOutDuplicationMap[this] = NewObject;
	NewObject->SourceObject = this;

	NewObject->SetOuter(InNewOuter);

	// 2. 소유하지 않는 참조 관계 얕은 복사
	NewObject->CopyPropertiesFrom(this);

	// 3. 소유하는 SubObject 깊은 복사
	NewObject->DuplicatesSubObjects(NewObject, InOutDuplicationMap);

	return NewObject;
}

void UObject::PostDuplicate(const TMap<UObject*, UObject*>& InDuplicationMap)
{
	// 'Objects' 맵의 포인터들을 재연결
	for (auto& Pair : this->Objects)
	{
		TObjectPtr<UObject>& RefObjectPtr = Pair.second;
		if (RefObjectPtr)
		{
			auto It = InDuplicationMap.find(RefObjectPtr.Get());
			if (It != InDuplicationMap.end())
			{
				RefObjectPtr = It->second;
			}
		}
	}

	// 'SubObjects' 맵에 있는 자식들에게도 재귀적으로 PostDuplicate 호출
	for (auto& Pair : this->SubObjects)
	{
		if (Pair.second)
		{
			Pair.second->PostDuplicate(InDuplicationMap);
		}
	}
}

void UObject::DuplicatesSubObjects(UObject* InNewOuter, TMap<UObject*, UObject*>& InOutDuplicationMap)
{
	if (SourceObject == nullptr) { return; }

	for (const auto& Pair : SourceObject->SubObjects)
	{
		const FName& SubObjectName = Pair.first;
		const TObjectPtr<UObject>& OriginalSubObject = Pair.second;

		if (OriginalSubObject)
		{
			UObject* NewSubObject = OriginalSubObject->Duplicate(InNewOuter, InOutDuplicationMap);
			this->SubObjects[SubObjectName] = NewSubObject;
		}
	}
}

void UObject::CopyPropertiesFrom(const UObject* InObject)
{
	// 'Objects' 맵은 포인터만 그대로 복사 (얕은 복사)
	this->Objects = InObject->Objects;
}

void UObject::SetOuter(UObject* InObject)
{
	if (Outer == InObject)
	{
		return;
	}

	// 기존 Outer가 있었다면, 나의 전체 메모리 사용량을 빼달라고 전파
	// 새로운 Outer가 있다면, 나의 전체 메모리 사용량을 더해달라고 전파
	if (Outer)
	{
		Outer->PropagateMemoryChange(-static_cast<int64>(AllocatedBytes), -static_cast<int32>(AllocatedCounts));
	}

	Outer = InObject;

	if (Outer)
	{
		Outer->PropagateMemoryChange(AllocatedBytes, AllocatedCounts);
	}
}

void UObject::AddMemoryUsage(uint64 InBytes, uint32 InCount)
{
	uint64 BytesToAdd = InBytes;

	if (!BytesToAdd)
	{
		BytesToAdd = GetClass()->GetClassSize();
	}

	// 메모리 변경 전파
	PropagateMemoryChange(BytesToAdd, InCount);
}

void UObject::RemoveMemoryUsage(uint64 InBytes, uint32 InCount)
{
	PropagateMemoryChange(-static_cast<int64>(InBytes), -static_cast<int32>(InCount));
}

void UObject::PropagateMemoryChange(uint64 InBytesDelta, uint32 InCountDelta)
{
	// 자신의 값에 변화량을 더함
	AllocatedBytes += InBytesDelta;
	AllocatedCounts += InCountDelta;

	// Outer가 있다면, 동일한 변화량을 그대로 전파
	if (Outer)
	{
		Outer->PropagateMemoryChange(InBytesDelta, InCountDelta);
	}
}

/**
 * @brief 해당 클래스가 현재 내 클래스의 조상 클래스인지 판단하는 함수
 * 내부적으로 재귀를 활용해서 부모를 계속 탐색한 뒤 결과를 반환한다
 * @param InClass 판정할 Class
 * @return 판정 결과
 */
bool UObject::IsA(TObjectPtr<UClass> InClass) const
{
	if (!InClass)
	{
		return false;
	}

	return GetClass()->IsChildOf(InClass);
}

/**
 * @brief 해당 클래스가 현재 내 클래스와 동일한지 판단하는 함수
 * @return 판정 결과
 */
bool UObject::IsExactly(TObjectPtr<UClass> InClass) const
{
	if (!InClass)
	{
		return false;
	}

	return GetClass() == InClass;
}

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