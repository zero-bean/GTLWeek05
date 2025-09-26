#include "pch.h"
#include "Global/Octree.h"
#include "Component/Public/PrimitiveComponent.h"

FOctree::FOctree()
{
	BoundingBox = { FVector(0, 0, 0), FVector(1500, 1500, 1500) };
	Depth = 0;
	for (int Index = 0; Index < 8; ++Index) { Children[Index] = nullptr; }
}

FOctree::FOctree(const FBoundingBox& InBoundingBox, int InDepth)
	: BoundingBox(InBoundingBox), Depth(InDepth)
{
	for (int Index = 0; Index < 8; ++Index) { Children[Index] = nullptr; }
}

FOctree::~FOctree()
{
	Primitives.clear();
	for (int Index = 0; Index < 8; ++Index) { SafeDelete(Children[Index]); }
}

void FOctree::Insert(UPrimitiveComponent* InPrimitive)
{
	// nullptr 체크
	if (!InPrimitive) { return; }

	// 0. 영역 내에 객체가 없으면 종료
	if (BoundingBox.IsIntersected(GetPrimitiveBoundingBox(InPrimitive)) == false) { return; }

	if (IsLeaf())
	{
		// 리프 노드이며, 여유 공간이 있거나 최대 깊이에 도달했다면
		if (Primitives.size() < MAX_PRIMITIVES || Depth == MAX_DEPTH)
		{
			Primitives.push_back(InPrimitive); // 해당 객체를 추가한다
		}
		else // 여유 공간이 없고, 최대 깊이에 도달하지 않았다면
		{
			// 분할 및 재귀적 추가를 한다
			Subdivide();
			Insert(InPrimitive);
		}
	}
	else
	{
		for (int Index = 0; Index < 8; ++Index)
		{
			// 자식 노드를 보유하고 있고, 영역 내에 해당 객체가 존재한다면
			if (Children[Index]->BoundingBox.IsIntersected(GetPrimitiveBoundingBox(InPrimitive)))
			{
				Children[Index]->Insert(InPrimitive); // 자식 노드에게 넘겨준다
			}
		}
	}
}

bool FOctree::Remove(UPrimitiveComponent* InPrimitive)
{
	if (InPrimitive == nullptr) { return false; }

	// 0. 현재 노드와 프리미티브가 겹치지 않으면, 탐색 종료
	if (!BoundingBox.IsIntersected(GetPrimitiveBoundingBox(InPrimitive))) { return false; }

	// 1-A. 리프 노드인 경우, 직접 프리미티브 목록에서 제거 시도
	if (IsLeaf())
	{
		// 리프 노드 내에 대상을 발견한다면, 마지막 요소 위치로 옮긴 뒤 삭제
		if (auto It = std::find(Primitives.begin(), Primitives.end(), InPrimitive); It != Primitives.end())
		{
			*It = std::move(Primitives.back());
			Primitives.pop_back();

			return true;
		}

		return false; // 없으므로 탐색 종료
	}
	// 1-B. 자식 노드가 있는 경우, 순차적으로 자식 노드 내부를 탐색
	else
	{
		bool bIsRemoved = false;

		for (int Index = 0; Index < 8; ++Index)
		{
			if (Children[Index]->Remove(InPrimitive))
			{
				bIsRemoved = true;
				break;
			}
		}

		// 4. 자식 노드에서 무언가 제거되었다면, 현재 노드를 합칠 수 있는지 검사
		if (bIsRemoved) { TryMerge(); }

		return bIsRemoved;
	}
}

void FOctree::Clear()
{
	Primitives.clear();
	for (int i = 0; i < 8; ++i) { SafeDelete(Children[i]); }
}

void FOctree::GetAllPrimitives(const FBoundingBox& InBoundingBox, TArray<UPrimitiveComponent*>& OutPrimitives)
{
	// 1. 현재 노드의 바운딩 박스와 입력된 바운딩 박스가 교차하지 않으면, 더 이상 탐색할 필요가 없습니다.
	if (!BoundingBox.IsIntersected(InBoundingBox))
	{
		return;
	}

	// 2. 현재 노드에 있는 모든 프리미티브에 대해, 입력된 바운딩 박스와 교차하는지 확인합니다.
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive)
		{
			FBoundingBox PrimitiveBoundingBox = GetPrimitiveBoundingBox(Primitive);
			if (PrimitiveBoundingBox.IsIntersected(InBoundingBox))
			{
				OutPrimitives.push_back(Primitive);
			}
		}
	}

	// 3. 리프 노드가 아니라면, 모든 자식 노드에 대해 재귀적으로 탐색을 계속합니다.
	if (!IsLeaf())
	{
		for (int Index = 0; Index < 8; ++Index)
		{
			if (Children[Index])
			{
				Children[Index]->GetAllPrimitives(InBoundingBox, OutPrimitives);
			}
		}
	}
}

void FOctree::GetAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	// 1. 현재 노드가 가진 프리미티브를 결과 배열에 추가합니다.
	OutPrimitives.insert(OutPrimitives.end(), Primitives.begin(), Primitives.end());

	// 2. 리프 노드가 아니라면, 모든 자식 노드에 대해 재귀적으로 함수를 호출합니다.
	if (!IsLeaf())
	{
		for (int Index = 0; Index < 8; ++Index)
		{
			if (Children[Index]) { Children[Index]->GetAllPrimitives(OutPrimitives); }
		}
	}
}

FBoundingBox FOctree::GetPrimitiveBoundingBox(UPrimitiveComponent* InPrimitive)
{
	FBoundingBox Result;
	FVector Min, Max;

	InPrimitive->GetWorldAABB(Min, Max);

	Result.Center = (Min + Max) * 0.5f;
	Result.Extent = (Max - Min) * 0.5f;

	return Result;
}

void FOctree::Subdivide()
{
	FVector ParentCenter = BoundingBox.Center;
	FVector ChildExtent = BoundingBox.Extent * 0.5f;

	Children[0] = new FOctree(
		FBoundingBox(ParentCenter + FVector(-ChildExtent.X, ChildExtent.Y, -ChildExtent.Z), ChildExtent),
		Depth + 1);

	Children[1] = new FOctree(
		FBoundingBox(ParentCenter + FVector(ChildExtent.X, ChildExtent.Y, -ChildExtent.Z), ChildExtent),
		Depth + 1);

	Children[2] = new FOctree(
		FBoundingBox(ParentCenter + FVector(-ChildExtent.X, ChildExtent.Y, ChildExtent.Z), ChildExtent),
		Depth + 1);

	Children[3] = new FOctree(
		FBoundingBox(ParentCenter + FVector(ChildExtent.X, ChildExtent.Y, ChildExtent.Z), ChildExtent),
		Depth + 1);

	Children[4] = new FOctree(
		FBoundingBox(ParentCenter + FVector(-ChildExtent.X, -ChildExtent.Y, -ChildExtent.Z), ChildExtent),
		Depth + 1);

	Children[5] = new FOctree(
		FBoundingBox(ParentCenter + FVector(ChildExtent.X, -ChildExtent.Y, -ChildExtent.Z), ChildExtent),
		Depth + 1);

	Children[6] = new FOctree(
		FBoundingBox(ParentCenter + FVector(-ChildExtent.X, -ChildExtent.Y, ChildExtent.Z), ChildExtent),
		Depth + 1);

	Children[7] = new FOctree(
		FBoundingBox(ParentCenter + FVector(ChildExtent.X, -ChildExtent.Y, ChildExtent.Z), ChildExtent),
		Depth + 1);


	for (int Index = 0; Index < 8; ++Index)
	{
		Children[Index]->BoundingBox.Extent = ChildExtent;
	}

	TArray<UPrimitiveComponent*> primitivesToMove = Primitives;
	Primitives.clear();

	for (UPrimitiveComponent* prim : primitivesToMove)
	{
		Insert(prim);
	}
}

void FOctree::TryMerge()
{
	// Case 1. 자식 노드가 존재하지 않으므로 종료
	if (IsLeaf()) { return; }

	// 모든 자식 노드에 있는 프리미티브의 총 개수를 계산
	uint32 TotalPrimitives = 0;
	for (int Index = 0; Index < 8; ++Index)
	{
		// 주의: 자식 노드가 또 다른 내부 노드일 수 있으므로, 재귀적으로 개수를 세면 안됩니다.
		// 합치기 조건은 오직 '모든 자식이 리프 노드'일 때 고려하는 것이 간단하고 일반적입니다.
		// 여기서는 모든 자식이 리프 노드라는 가정 하에 개수만 셉니다.
		TotalPrimitives += Children[Index]->Primitives.size();
	}

	// 프리미티브 총 개수가 최대치보다 작으면 합치기 수행
	if (TotalPrimitives <= MAX_PRIMITIVES)
	{
		for (int Index = 0; Index < 8; ++Index)
		{
			Primitives.insert(Primitives.end(), Children[Index]->Primitives.begin(), Children[Index]->Primitives.end());
		}

		// 모든 자식 노드를 메모리에서 해제
		for (int Index = 0; Index < 8; ++Index) { SafeDelete(Children[Index]); }
	}
}
