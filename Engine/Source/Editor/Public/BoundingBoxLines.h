#pragma once
#include "Core/Public/Object.h"
#include "Physics/Public/AABB.h"

class UBoundingBoxLines : public UObject
{
    DECLARE_CLASS(UBoundingBoxLines, UObject)
public:
	UBoundingBoxLines();
	~UBoundingBoxLines() = default;

	void MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex);
	void UpdateVertices(FAABB BoundingBoxInfo);

	uint32 GetNumVertices() const { return NumVertices; }

	FAABB GetRenderedBoxInfo() const { return RenderedBoxInfo; }

private:
	TArray<FVector> Vertices;
	uint32 NumVertices;
	FAABB RenderedBoxInfo;
};

