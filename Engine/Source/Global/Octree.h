#pragma once

#include "Physics/Public/AABB.h"

class UPrimitiveComponent;

constexpr int MAX_PRIMITIVES = 32; 
constexpr int MAX_DEPTH = 5;      

struct FLinearOctreeNode
{
	// 이 노드가 담당하는 프리미티브들의 시작 인덱스
	uint32 StartIndex = 0;
	// 이 노드가 담당하는 프리미티브들의 개수
	uint32 PrimitiveCount = 0;
	// 자식 노드들의 인덱스. -1이면 리프 노드
	int32 Children[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
};

class FOctree
{
public:
	FOctree();
	FOctree(const FVector& InPosition, float InSize, int InDepth);
	FOctree(const FAABB& InBoundingBox, int InDepth);
	~FOctree();

	bool Insert(UPrimitiveComponent* InPrimitive);
	bool Remove(UPrimitiveComponent* InPrimitive);
	void Clear();

	void GetAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;
	TArray<UPrimitiveComponent*> FindNearestPrimitives(const FVector& FindPos, uint32 MaxPrimitiveCount);

	const FAABB& GetBoundingBox() const { return BoundingBox; }
	bool IsLeafNode() const { return IsLeaf(); }
	const TArray<UPrimitiveComponent*>& GetPrimitives() const { return Primitives; }
	TArray<FOctree*>& GetChildren() { return Children; }

private:
	bool IsLeaf() const { return Children[0] == nullptr; }
	void Subdivide(UPrimitiveComponent* InPrimitive);
	void TryMerge();

	FAABB BoundingBox;
	int Depth;                       
	TArray<UPrimitiveComponent*> Primitives;
	TArray<FOctree*> Children;
};

using FNodeQueue = std::priority_queue<
	std::pair<float, FOctree*>,
	std::vector<std::pair<float, FOctree*>>,
	std::greater<std::pair<float, FOctree*>>
>;

class FLinearOctree
{
public:
	FLinearOctree();
	FLinearOctree(const FVector& InPosition, float InSize, int InMaxDepth = MAX_DEPTH);
	~FLinearOctree();

	// 인터페이스 유지를 위한 Insert 함수 (내부적으로는 Rebuild를 호출)
	bool Insert(UPrimitiveComponent* InPrimitive);
	// 인터페이스 유지를 위한 Remove 함수 (성능 저하의 주범이 될 수 있음)
	bool Remove(UPrimitiveComponent* InPrimitive);

	// 모든 프리미티브를 받아 트리를 구성하는 핵심 함수
	void Build(const TArray<UPrimitiveComponent*>& InPrimitives);

	// 기존 인터페이스와 동일한 쿼리 함수들
	void GetAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;
	// FindNearestPrimitives는 선형 옥트리에서 구현이 복잡하므로, 특정 영역 내의 프리미티브를 찾는 함수로 대체 제안
	void FindPrimitivesInBounds(const FAABB& InBounds, TArray<UPrimitiveComponent*>& OutPrimitives) const;

	const FAABB& GetBoundingBox() const { return RootBoundingBox; }

private:
	// 재귀적으로 노드를 생성하는 함수
	void BuildRecursive(uint32 NodeIndex, uint32 MortonCodeStart, uint32 MortonCodeEnd, int InDepth);
	// 특정 영역 내 프리미티브를 재귀적으로 찾는 함수
	void FindPrimitivesInBoundsRecursive(uint32 NodeIndex, const FAABB& InBounds, TArray<UPrimitiveComponent*>& OutPrimitives) const;

	// 모튼 코드를 계산하는 헬퍼 함수
	uint32 MortonCode3D(const FVector& Point);
	// 10비트 정수를 30비트로 확장 (모튼 코드 계산용)
	uint32 ExpandBits(uint32 v);

private:
	// 월드의 전체 영역
	FAABB RootBoundingBox;
	// 최대 깊이
	int MaxDepth;

	// 모든 노드를 저장하는 배열 (포인터 대신 인덱스 사용)
	TArray<FLinearOctreeNode> Nodes;

	// 원본 프리미티브 포인터 배열 (외부에서 관리)
	TArray<UPrimitiveComponent*> Primitives;
	// 프리미티브의 모튼 코드와 인덱스를 저장하는 배열 (정렬에 사용)
	TArray<std::pair<uint32, int>> MortonSortedPrimitives;
};