#pragma once

/*
	자식 노드 전략

	자식 노드는 7가지 중 하나의 형태를 띌 수 있다.

	1. 정육면체 = 초기 루트노드 모양 그 자체
	2. 3. 4) 정육면체를 xy, yz, xz 평면으로 2등분한 것
	5. 6. 7) 2, 3, 4를 자기가 잘리지 않은 평면으로 이등분 것
	8. 5. 6. 7.은 1의 형태로 밖에 분할될 수 밖에 없다.

	부모 노드는 다음과 같은 경우에 자식 노드를 가진다.

	1. 스스로가 1개 이상의 렌더링 대상를 가질 때
	2. 그 중 자신이 잘리지 않은 평면에 교차하지 않는 것이 적어도 하나 있을 때
	3. 단, 스스로가 정육면체의 형태를 하고 있으면서 크기가 최소 분할 제한과 같으면 분할하지 않는다.
	4. 최소 분할 제한은 (root 노드의 크기) * (1 / 2)^4이다.
*/

enum class BSPType
{
	Cube,
	XYExtended,
	YZExtended,
	XZExtended,
	XExtended,
	YExtended,
	ZExtended,
};

struct BSPNode
{
	BSPType		Type = BSPType::Cube;
	FVector		Position;
	FVector		Extent;

	TArray<TObjectPtr<UPrimitiveComponent>> Primitives;

	/* Child Node */
	BSPNode*	Left = nullptr;
	BSPNode*	Right = nullptr;
};

class FBSP
{
public:

	void Initialize(const TArray<TObjectPtr<UPrimitiveComponent>>& LevelPrimitives);
	void Shutdown();

	BSPNode* GetRoot() { return Root;  };

	static void PreOrder(BSPNode* Node, const std::function<void(BSPNode*)>& Apply);
	static void PreOrderUntil(BSPNode* Node, const std::function<bool(BSPNode*)>& Apply);
	static void InOrder(BSPNode* Node, const std::function<void(BSPNode*)>& Apply);
	static void PostOrder(BSPNode* Node, const std::function<void(BSPNode*)>& Apply);
private:
	void DivideCube(BSPNode* Cube);
	void DevideXYExtended(BSPNode* XYExtended);
	void DevideYZExtended(BSPNode* YZExtended);
	void DevideXZExtended(BSPNode* XZExtended);
	void DevideXExtended(BSPNode* XExtended);
	void DevideYExtended(BSPNode* YExtended);
	void DevideZExtended(BSPNode* ZExtended);
	void Divide(BSPNode* Node);

	bool IntersectWithXYPlane(const float ZMin, const float ZMax);
	bool IntersectWithYZPlane(const float XMin, const float XMax);
	bool IntersectWithXZPlane(const float YMin, const float YMax);

	uint32 CountXYIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives);
	uint32 CountYZIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives);
	uint32 CountXZIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives);

	void DivideWithXYPlane(
		BSPNode* N,
		const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
		TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
	);
	void DivideWithYZPlane(
		BSPNode* N,
		const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
		TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
	);
	void DivideWithXZPlane(
		BSPNode* N,
		const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
		TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
	);

	BSPNode* Root = nullptr;
	int MinimumExtent = 0;
};
