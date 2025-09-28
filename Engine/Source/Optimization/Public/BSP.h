#pragma once

#include "Component/Public/PrimitiveComponent.h"

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

	void Remove(const TObjectPtr<UPrimitiveComponent>& Target);
	bool Insert(const TObjectPtr<UPrimitiveComponent>& Target);

	void RenderTree() const;

	BSPNode* GetRoot() { return Root; };
	const BSPNode* GetRoot() const { return Root; };

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

	bool IntersectWithXYPlane(const float ZMin, const float ZMax) const;
	bool IntersectWithYZPlane(const float XMin, const float XMax) const;
	bool IntersectWithXZPlane(const float YMin, const float YMax) const;

	uint32 CountXYIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives) const;
	uint32 CountYZIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives) const;
	uint32 CountXZIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives) const;

	void DivideWithXYPlane(
		BSPNode* N,
		const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
		TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
	) const;
	void DivideWithYZPlane(
		BSPNode* N,
		const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
		TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
	) const;
	void DivideWithXZPlane(
		BSPNode* N,
		const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
		TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
		TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
	) const;

	void FindNodeToInsert(
		BSPNode* Node,
		const TObjectPtr<UPrimitiveComponent>& Target,
		const FVector& BoxMin,
		const FVector& BoxMax
		);

	BSPNode* Root = nullptr;
	int MinimumExtent = 0;
};
