#pragma once

#include "Physics/Public/AABB.h"

class UPrimitiveComponent;

constexpr int MAX_PRIMITIVES = 32; 
constexpr int MAX_DEPTH = 5;      

class FOctree
{
public:
	FOctree();
	FOctree(const FVector& InPosition, float InSize, int InDepth);
	FOctree(const FAABB& InBoundingBox, int InDepth);
	~FOctree();

	void Insert(UPrimitiveComponent* InPrimitive);
	bool Remove(UPrimitiveComponent* InPrimitive);
	void Clear();

	void GetAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;

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
