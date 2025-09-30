#include "pch.h"
#include "Editor/Public/BoundingBoxLines.h"
#include "Physics/Public/AABB.h"

IMPLEMENT_CLASS(UBoundingBoxLines, UObject)
UBoundingBoxLines::UBoundingBoxLines() : Vertices(TArray<FVector>()), NumVertices(8)
{
	Vertices.reserve(NumVertices);
	UpdateVertices(FAABB({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
}

void UBoundingBoxLines::MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex)
{
	// 인덱스 범위 보정
	if (insertStartIndex > destVertices.size())
		insertStartIndex = destVertices.size();

	// 미리 메모리 확보
	destVertices.reserve(destVertices.size() + std::distance(Vertices.begin(), Vertices.end()));

	// 덮어쓸 수 있는 개수 계산
	size_t overwriteCount = std::min(
		Vertices.size(),
		destVertices.size() - insertStartIndex
	);

	// 기존 요소 덮어쓰기
	std::copy(
		Vertices.begin(),
		Vertices.begin() + overwriteCount,
		destVertices.begin() + insertStartIndex
	);

	// 원하는 위치에 삽입
	/*destVertices.insert(
		destVertices.begin() + insertStartIndex,
		Vertices.begin(),
		Vertices.end()
	);*/
}

void UBoundingBoxLines::UpdateVertices(FAABB BoundingBoxInfo)
{
	// 중복 삽입 방지
	if (RenderedBoxInfo.Min == BoundingBoxInfo.Min && RenderedBoxInfo.Max == BoundingBoxInfo.Max)
	{
		return;
	}

	float MinX = BoundingBoxInfo.Min.X, MinY = BoundingBoxInfo.Min.Y, MinZ = BoundingBoxInfo.Min.Z;
	float MaxX = BoundingBoxInfo.Max.X, MaxY = BoundingBoxInfo.Max.Y, MaxZ = BoundingBoxInfo.Max.Z;

	if (Vertices.size() < NumVertices)
	{
		Vertices.resize(NumVertices);
	}

	// 꼭짓점 정의 (0~3: 앞면, 4~7: 뒷면)
	uint32 vertexIndex = 0;
	Vertices[vertexIndex++] = { MinX, MinY, MinZ }; // Front-Bottom-Left
	Vertices[vertexIndex++] = { MaxX, MinY, MinZ }; // Front-Bottom-Right
	Vertices[vertexIndex++] = { MaxX, MaxY, MinZ }; // Front-Top-Right
	Vertices[vertexIndex++] = { MinX, MaxY, MinZ }; // Front-Top-Left
	Vertices[vertexIndex++] = { MinX, MinY, MaxZ }; // Back-Bottom-Left
	Vertices[vertexIndex++] = { MaxX, MinY, MaxZ }; // Back-Bottom-Right
	Vertices[vertexIndex++] = { MaxX, MaxY, MaxZ }; // Back-Top-Right
	Vertices[vertexIndex++] = { MinX, MaxY, MaxZ }; // Back-Top-Left

	RenderedBoxInfo = BoundingBoxInfo;
}

