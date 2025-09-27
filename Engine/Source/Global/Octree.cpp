#include "pch.h"
#include "Global/Octree.h"
#include "Component/Public/PrimitiveComponent.h"

FOctree::FOctree()
{
	BoundingBox.Min = FVector(-200, -200, -5);
	BoundingBox.Max = FVector(200, 200, 55);
	Depth = 0;
	for (int Index = 0; Index < 8; ++Index) { Children[Index] = nullptr; }
}

FOctree::FOctree(const FAABB& InBoundingBox, int InDepth)
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
			Subdivide(InPrimitive);
		}
	}
	else
	{
		bool bWasInsertedIntoChild = false;
		for (int Index = 0; Index < 8; ++Index)
		{
			// 자식 노드를 보유하고 있고, 영역 내에 해당 객체가 존재한다면
			if (Children[Index]->BoundingBox.IsIntersected(GetPrimitiveBoundingBox(InPrimitive)))
			{
				Children[Index]->Insert(InPrimitive); // 자식 노드에게 넘겨준다
				bWasInsertedIntoChild = true;
				break;
			}
		}

		if (!bWasInsertedIntoChild)
		{
			Primitives.push_back(InPrimitive);
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

void FOctree::GetAllPrimitives(const FAABB& InBoundingBox, TArray<UPrimitiveComponent*>& OutPrimitives)
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
			const FAABB& PrimitiveBoundingBox = GetPrimitiveBoundingBox(Primitive);
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

FAABB FOctree::GetPrimitiveBoundingBox(UPrimitiveComponent* InPrimitive)
{
	FVector Min, Max;
	InPrimitive->GetWorldAABB(Min, Max);

	return FAABB(Min, Max);
}

void FOctree::Subdivide(UPrimitiveComponent* InPrimitive)
{
	const FVector Center = (BoundingBox.Min + BoundingBox.Max) * 0.5f;
	Children[0] = new FOctree(FAABB(FVector(BoundingBox.Min.X, Center.Y, BoundingBox.Min.Z), FVector(Center.X, BoundingBox.Max.Y, Center.Z)), Depth + 1); // Top-Back-Left
	Children[1] = new FOctree(FAABB(FVector(Center.X, Center.Y, BoundingBox.Min.Z), FVector(BoundingBox.Max.X, BoundingBox.Max.Y, Center.Z)), Depth + 1); // Top-Back-Right
	Children[2] = new FOctree(FAABB(FVector(BoundingBox.Min.X, Center.Y, Center.Z), FVector(Center.X, BoundingBox.Max.Y, BoundingBox.Max.Z)), Depth + 1); // Top-Front-Left
	Children[3] = new FOctree(FAABB(FVector(Center.X, Center.Y, Center.Z), FVector(BoundingBox.Max.X, BoundingBox.Max.Y, BoundingBox.Max.Z)), Depth + 1); // Top-Front-Right
	Children[4] = new FOctree(FAABB(FVector(BoundingBox.Min.X, BoundingBox.Min.Y, BoundingBox.Min.Z), FVector(Center.X, Center.Y, Center.Z)), Depth + 1); // Bottom-Back-Left
	Children[5] = new FOctree(FAABB(FVector(Center.X, BoundingBox.Min.Y, BoundingBox.Min.Z), FVector(BoundingBox.Max.X, Center.Y, Center.Z)), Depth + 1); // Bottom-Back-Right
	Children[6] = new FOctree(FAABB(FVector(BoundingBox.Min.X, BoundingBox.Min.Y, Center.Z), FVector(Center.X, Center.Y, BoundingBox.Max.Z)), Depth + 1); // Bottom-Front-Left
	Children[7] = new FOctree(FAABB(FVector(Center.X, BoundingBox.Min.Y, Center.Z), FVector(BoundingBox.Max.X, Center.Y, BoundingBox.Max.Z)), Depth + 1); // Bottom-Front-Right

	TArray<UPrimitiveComponent*> primitivesToMove = Primitives;
	primitivesToMove.push_back(InPrimitive);
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

	// 모든 자식 노드가 리프 노드인지 확인
	for (int Index = 0; Index < 8; ++Index)
	{
		if (!Children[Index]->IsLeaf())
		{
			return; // 하나라도 리프가 아니면 합치지 않음
		}
	}

	// 모든 자식 노드에 있는 프리미티브의 총 개수를 계산
	uint32 TotalPrimitives = Primitives.size();
	for (int Index = 0; Index < 8; ++Index)
	{
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
