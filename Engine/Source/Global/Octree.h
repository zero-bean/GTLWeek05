#pragma once

class UPrimitiveComponent;

constexpr int MAX_PRIMITIVES = 1000; 
constexpr int MAX_DEPTH = 5;      

struct FBoundingBox
{
	FBoundingBox() : Center(FVector::ZeroVector()), Extent(FVector::ZeroVector()) {}
	FBoundingBox(const FVector InCenter, const FVector InExtent) : Center(InCenter), Extent(InExtent) {}
	~FBoundingBox() {}

	bool IsIntersected(const FBoundingBox& Other) const
	{
		return (std::abs(Center.X - Other.Center.X) <= (Extent.X + Other.Extent.X)) &&
			(std::abs(Center.Y - Other.Center.Y) <= (Extent.Y + Other.Extent.Y)) &&
			(std::abs(Center.Z - Other.Center.Z) <= (Extent.Z + Other.Extent.Z));
	}

	FVector Center = FVector::ZeroVector();
	FVector Extent = FVector::ZeroVector(); 
};

class FOctree
{
public:
	FOctree();
	FOctree(const FBoundingBox& InBoundingBox, int InDepth);
	~FOctree();

	void Insert(UPrimitiveComponent* InPrimitive);
	bool Remove(UPrimitiveComponent* InPrimitive);
	void Clear();
	void GetAllPrimitives(const FBoundingBox& InBoundingBox, TArray<UPrimitiveComponent*>& OutPrimitives);
	void GetAllPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;

private:
	FBoundingBox GetPrimitiveBoundingBox(UPrimitiveComponent* InPrimitive);
	void Subdivide(UPrimitiveComponent* InPrimitive);
	bool IsLeaf() const { return Children[0] == nullptr; }
	void TryMerge();

	FBoundingBox BoundingBox;       
	int Depth;                       
	TArray<UPrimitiveComponent*> Primitives;
	FOctree* Children[8];
};
