#include "pch.h"
#include "Global/Octree.h"
#include "Component/Public/PrimitiveComponent.h"

namespace
{
	FAABB GetPrimitiveBoundingBox(UPrimitiveComponent* InPrimitive)
	{
		FVector Min, Max;
		InPrimitive->GetWorldAABB(Min, Max);

		return FAABB(Min, Max);
	}
}

FOctree::FOctree()
	: BoundingBox(), Depth(0)
{
	Children.resize(8);
}

FOctree::FOctree(const FAABB& InBoundingBox, int InDepth)
	: BoundingBox(InBoundingBox), Depth(InDepth)
{
	Children.resize(8);
}

FOctree::FOctree(const FVector& InPosition, float InSize, int InDepth)
	: Depth(InDepth)
{
	const float HalfSize = InSize * 0.5f;
	BoundingBox.Min = InPosition - FVector(HalfSize, HalfSize, HalfSize);
	BoundingBox.Max = InPosition + FVector(HalfSize, HalfSize, HalfSize);
	Children.resize(8);
}

FOctree::~FOctree()
{
	Primitives.clear();
	for (int Index = 0; Index < 8; ++Index) { SafeDelete(Children[Index]); }
}

bool FOctree::Insert(UPrimitiveComponent* InPrimitive)
{
	// nullptr 체크
	if (!InPrimitive) { return false; }

	// 0. 영역 내에 객체가 없으면 종료
	if (BoundingBox.IsIntersected(GetPrimitiveBoundingBox(InPrimitive)) == false) { return false; }

	if (IsLeaf())
	{
		// 리프 노드이며, 여유 공간이 있거나 최대 깊이에 도달했다면
		if (Primitives.size() < MAX_PRIMITIVES || Depth == MAX_DEPTH)
		{
			Primitives.push_back(InPrimitive); // 해당 객체를 추가한다
			return true;
		}
		else // 여유 공간이 없고, 최대 깊이에 도달하지 않았다면
		{
			// 분할 및 재귀적 추가를 한다
			Subdivide(InPrimitive);
			return true;
		}
	}
	else
	{
		for (int Index = 0; Index < 8; ++Index)
		{
			// 자식 노드를 보유하고 있고, 영역 내에 해당 객체가 존재한다면
			if (Children[Index] && Children[Index]->BoundingBox.IsContains(GetPrimitiveBoundingBox(InPrimitive)))
			{
				return Children[Index]->Insert(InPrimitive); // 자식 노드에게 넘겨준다
			}
		}

		Primitives.push_back(InPrimitive);
		return true;
	}

	return false;
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
	for (int Index = 0; Index < 8; ++Index) { SafeDelete(Children[Index]); }
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

TArray<UPrimitiveComponent*> FOctree::FindNearestPrimitives(const FVector& FindPos, uint32 MaxPrimitiveCount)
{
	TArray<UPrimitiveComponent*> Candidates; Candidates.reserve(MaxPrimitiveCount);
	FNodeQueue NodeQueue;

	float RootDistance = this->GetBoundingBox().GetCenterDistanceSquared(FindPos);
	NodeQueue.push({ RootDistance, this });

	while (!NodeQueue.empty() && Candidates.size() < MaxPrimitiveCount)
	{
		FOctree* CurrentNode = NodeQueue.top().second;
		NodeQueue.pop();

		if (CurrentNode->IsLeafNode())
		{
			for (UPrimitiveComponent* Primitive : CurrentNode->GetPrimitives())
			{
				Candidates.push_back(Primitive);
			}
		}
		else
		{
			for (int i = 0; i < 8; ++i)
			{
				FOctree* Child = CurrentNode->Children[i];
				if (Child)
				{
					float ChildDistance = Child->GetBoundingBox().GetCenterDistanceSquared(FindPos);
					NodeQueue.push({ ChildDistance, Child });
				}
			}
		}
	}

	return Candidates;
}

void FOctree::Subdivide(UPrimitiveComponent* InPrimitive)
{
	const FVector& Min = BoundingBox.Min;
	const FVector& Max = BoundingBox.Max;
	const FVector Center = (Min + Max) * 0.5f;

	Children[0] = new FOctree(FAABB(FVector(Min.X, Center.Y, Min.Z), FVector(Center.X, Max.Y, Center.Z)), Depth + 1); // Top-Back-Left
	Children[1] = new FOctree(FAABB(FVector(Center.X, Center.Y, Min.Z), FVector(Max.X, Max.Y, Center.Z)), Depth + 1); // Top-Back-Right
	Children[2] = new FOctree(FAABB(FVector(Min.X, Center.Y, Center.Z), FVector(Center.X, Max.Y, Max.Z)), Depth + 1); // Top-Front-Left
	Children[3] = new FOctree(FAABB(FVector(Center.X, Center.Y, Center.Z), FVector(Max.X, Max.Y, Max.Z)), Depth + 1); // Top-Front-Right
	Children[4] = new FOctree(FAABB(FVector(Min.X, Min.Y, Min.Z), FVector(Center.X, Center.Y, Center.Z)), Depth + 1); // Bottom-Back-Left
	Children[5] = new FOctree(FAABB(FVector(Center.X, Min.Y, Min.Z), FVector(Max.X, Center.Y, Center.Z)), Depth + 1); // Bottom-Back-Right
	Children[6] = new FOctree(FAABB(FVector(Min.X, Min.Y, Center.Z), FVector(Center.X, Center.Y, Max.Z)), Depth + 1); // Bottom-Front-Left
	Children[7] = new FOctree(FAABB(FVector(Center.X, Min.Y, Center.Z), FVector(Max.X, Center.Y, Max.Z)), Depth + 1); // Bottom-Front-Right

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FLinearOctree::FLinearOctree()
	: RootBoundingBox(), MaxDepth(8)
{
}

FLinearOctree::FLinearOctree(const FVector& InPosition, float InSize, int InMaxDepth)
	: MaxDepth(InMaxDepth)
{
	const float HalfSize = InSize * 0.5f;
	RootBoundingBox.Min = InPosition - FVector(HalfSize, HalfSize, HalfSize);
	RootBoundingBox.Max = InPosition + FVector(HalfSize, HalfSize, HalfSize);
}

FLinearOctree::~FLinearOctree()
{
}

bool FLinearOctree::Insert(UPrimitiveComponent* InPrimitive)
{
	// 선형 옥트리는 개별 삽입이 비효율적이므로, 기존 데이터에 추가하고 전체를 재빌드
	Primitives.push_back(InPrimitive);
	Build(Primitives); // 비효율적일 수 있으나 인터페이스 유지를 위해 사용
	return true;
}

bool FLinearOctree::Remove(UPrimitiveComponent* InPrimitive)
{
	// 개별 삭제 역시 매우 비효율적
	auto It = std::remove(Primitives.begin(), Primitives.end(), InPrimitive);
	if (It != Primitives.end())
	{
		Primitives.erase(It, Primitives.end());
		Build(Primitives); // 재빌드
		return true;
	}
	return false;
}

void FLinearOctree::Build(const TArray<UPrimitiveComponent*>& InPrimitives)
{
	Primitives = InPrimitives;
	Nodes.clear();
	MortonSortedPrimitives.clear();

	if (Primitives.empty()) return;

	// 1. 모든 프리미티브에 대해 모튼 코드 계산
	MortonSortedPrimitives.reserve(Primitives.size());
	for (int i = 0; i < Primitives.size(); ++i)
	{
		FVector Center = GetPrimitiveBoundingBox(Primitives[i]).GetCenter();
		MortonSortedPrimitives.push_back({ MortonCode3D(Center), i });
	}

	// 2. 모튼 코드를 기준으로 정렬
	std::sort(MortonSortedPrimitives.begin(), MortonSortedPrimitives.end(),
		[](const auto& a, const auto& b) {
			return a.first < b.first;
		});

	// 3. 재귀적으로 노드 계층 구조 생성
	Nodes.emplace_back(); // 루트 노드 추가
	BuildRecursive(0, 0, Primitives.size(), 0);
}

void FLinearOctree::GetAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	OutPrimitives.reserve(OutPrimitives.size() + Primitives.size());
	for (const auto& pair : MortonSortedPrimitives)
	{
		OutPrimitives.push_back(Primitives[pair.second]);
	}
}


void FLinearOctree::FindPrimitivesInBounds(const FAABB& InBounds, TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	if (!Nodes.empty())
	{
		FindPrimitivesInBoundsRecursive(0, InBounds, OutPrimitives);
	}
}

void FLinearOctree::BuildRecursive(uint32 NodeIndex, uint32 MortonCodeStart, uint32 MortonCodeEnd, int InDepth)
{
	// 현재 노드에 프리미티브 정보 할당
	Nodes[NodeIndex].StartIndex = MortonCodeStart;
	Nodes[NodeIndex].PrimitiveCount = MortonCodeEnd - MortonCodeStart;

	if (Nodes[NodeIndex].PrimitiveCount <= 1 || InDepth >= MaxDepth)
	{
		// 리프 노드 조건 충족 시 종료
		return;
	}

	// 자식 노드로 분할될 지점을 찾음
	uint32 Splits[8] = { 0 };
	uint32 Current = MortonCodeStart;

	for (int i = 0; i < 7; ++i)
	{
		uint32 SplitCode = (i + 1) << (3 * (MaxDepth - InDepth - 1));
		auto It = std::lower_bound(MortonSortedPrimitives.begin() + Current, MortonSortedPrimitives.begin() + MortonCodeEnd, SplitCode,
			[InDepth, this](const auto& a, uint32 code) {
				// 현재 깊이에 해당하는 비트만 비교
				return (a.first >> (3 * (MaxDepth - InDepth - 1))) < (code >> (3 * (MaxDepth - InDepth - 1)));
			});

		Splits[i] = std::distance(MortonSortedPrimitives.begin(), It);
		Current = Splits[i];
	}

	uint32 Start = MortonCodeStart;
	for (int i = 0; i < 8; ++i)
	{
		uint32 End = (i == 7) ? MortonCodeEnd : Splits[i];
		if (Start >= End) continue;

		uint32 ChildNodeIndex = Nodes.size();
		Nodes[NodeIndex].Children[i] = ChildNodeIndex;
		Nodes.emplace_back();

		BuildRecursive(ChildNodeIndex, Start, End, InDepth + 1);
		Start = End;
	}
}

void FLinearOctree::FindPrimitivesInBoundsRecursive(uint32 NodeIndex, const FAABB& InBounds, TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	const FLinearOctreeNode& Node = Nodes[NodeIndex];

	// 리프 노드인 경우
	bool isLeaf = true;
	for (int child : Node.Children)
	{
		if (child != -1)
		{
			isLeaf = false;
			break;
		}
	}

	if (isLeaf)
	{
		for (uint32 i = 0; i < Node.PrimitiveCount; ++i)
		{
			int primitiveIndex = MortonSortedPrimitives[Node.StartIndex + i].second;
			if (InBounds.IsIntersected(GetPrimitiveBoundingBox(Primitives[primitiveIndex])))
			{
				OutPrimitives.push_back(Primitives[primitiveIndex]);
			}
		}
		return;
	}

	// 자식 노드 순회 (구현 간소화를 위해 모든 자식 체크, 최적화 가능)
	for (int i = 0; i < 8; ++i)
	{
		if (Node.Children[i] != -1)
		{
			// TODO: 자식 노드의 BoundingBox와 InBounds의 교차 검사 로직 추가 필요
			// 여기서는 간소화를 위해 무조건 재귀 호출
			FindPrimitivesInBoundsRecursive(Node.Children[i], InBounds, OutPrimitives);
		}
	}
}

// 3D 좌표로부터 30비트 모튼 코드를 생성
uint32 FLinearOctree::MortonCode3D(const FVector& Point)
{
	// 1. 좌표를 0~1 범위로 정규화
	FVector Distance = Point - RootBoundingBox.Min;
	FVector TotalBoxDistance = RootBoundingBox.Max - RootBoundingBox.Min;
	FVector RelativePos = { Distance.X / TotalBoxDistance.X, Distance.Y / TotalBoxDistance.Y, Distance.Z / TotalBoxDistance.Z };

	// 2. 0~1023 (10비트) 범위로 스케일링
	RelativePos.X = std::min(std::max(RelativePos.X * 1023.0f, 0.0f), 1023.0f);
	RelativePos.Y = std::min(std::max(RelativePos.Y * 1023.0f, 0.0f), 1023.0f);
	RelativePos.Z = std::min(std::max(RelativePos.Z * 1023.0f, 0.0f), 1023.0f);

	// 3. 각 축의 값을 확장하고 비트 연산으로 합침 (Z-Y-X 순서)
	uint32 xx = ExpandBits(static_cast<uint32>(RelativePos.X));
	uint32 yy = ExpandBits(static_cast<uint32>(RelativePos.Y));
	uint32 zz = ExpandBits(static_cast<uint32>(RelativePos.Z));
	return xx | (yy << 1) | (zz << 2);
}


uint32 FLinearOctree::ExpandBits(uint32 v)
{
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;
	return v;
}