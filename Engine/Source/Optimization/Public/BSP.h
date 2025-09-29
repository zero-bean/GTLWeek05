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
	void DivideXYExtended(BSPNode* XYExtended);
	void DivideYZExtended(BSPNode* YZExtended);
	void DivideXZExtended(BSPNode* XZExtended);
	void DivideXExtended(BSPNode* XExtended);
	void DivideYExtended(BSPNode* YExtended);
	void DivideZExtended(BSPNode* ZExtended);
	void Divide(BSPNode* Node);

	bool IntersectWithXYPlane(BSPNode* Node, const float ZMin, const float ZMax) const;
	bool IntersectWithYZPlane(BSPNode* Node, const float XMin, const float XMax) const;
	bool IntersectWithXZPlane(BSPNode* Node, const float YMin, const float YMax) const;

	uint32 CountXYIntersected(BSPNode* Node) const;
	uint32 CountYZIntersected(BSPNode* Node) const;
	uint32 CountXZIntersected(BSPNode* Node) const;

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
	float MinimumExtent = 0.0f;
};
