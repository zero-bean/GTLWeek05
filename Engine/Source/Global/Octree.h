#pragma once

#include "Physics/Public/AABB.h"

class UPrimitiveComponent;

constexpr int MAX_PRIMITIVES = 32; 
constexpr int MAX_DEPTH = 5;      

class FOctree
{
public:
	FOctree();
	FOctree(const FAABB& InBoundingBox, int InDepth);
	~FOctree();

	void Insert(UPrimitiveComponent* InPrimitive);
	bool Remove(UPrimitiveComponent* InPrimitive);
	void Clear();
	void GetAllPrimitives(const FAABB& InBoundingBox, TArray<UPrimitiveComponent*>& OutPrimitives);
	void GetAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;

private:
	FAABB GetPrimitiveBoundingBox(UPrimitiveComponent* InPrimitive);
	void Subdivide(UPrimitiveComponent* InPrimitive);
	bool IsLeaf() const { return Children[0] == nullptr; }
	void TryMerge();

	FAABB BoundingBox;
	int Depth;                       
	TArray<UPrimitiveComponent*> Primitives;
	FOctree* Children[8];
};
