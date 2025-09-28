#pragma once

/*
	�ڽ� ��� ����

	�ڽ� ���� 7���� �� �ϳ��� ���¸� �� �� �ִ�.

	1. ������ü = �ʱ� ��Ʈ��� ��� �� ��ü
	2. 3. 4) ������ü�� xy, yz, xz ������� 2����� ��
	5. 6. 7) 2, 3, 4�� �ڱⰡ �߸��� ���� ������� �̵�� ��
	8. 5. 6. 7.�� 1�� ���·� �ۿ� ���ҵ� �� �ۿ� ����.

	�θ� ���� ������ ���� ��쿡 �ڽ� ��带 ������.

	1. �����ΰ� 1�� �̻��� ������ ��� ���� ��
	2. �� �� �ڽ��� �߸��� ���� ��鿡 �������� �ʴ� ���� ��� �ϳ� ���� ��
	3. ��, �����ΰ� ������ü�� ���¸� �ϰ� �����鼭 ũ�Ⱑ �ּ� ���� ���Ѱ� ������ �������� �ʴ´�.
	4. �ּ� ���� ������ (root ����� ũ��) * (1 / 2)^4�̴�.
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
