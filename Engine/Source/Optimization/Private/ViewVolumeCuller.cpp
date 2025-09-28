#include "pch.h"
#include "Optimization/Public/ViewVolumeCuller.h"
#include "Core/Public/Object.h"
#include "Global/Octree.h"

namespace
{
	FAABB GetPrimitiveBoundingBox(UPrimitiveComponent* InPrimitive)
	{
		FVector Min, Max;
		InPrimitive->GetWorldAABB(Min, Max);

		return FAABB(Min, Max);
	}
}

void ViewVolumeCuller::Cull(FOctree* StaticOctree, FOctree* DynamicOctree, const FViewProjConstants& ViewProjConstants)
{
	// 이전의 Cull했던 정보를 지운다.
	RenderableObjects.clear();
	CurrentFrustum.Clear();

	// 1. 절두체 'Key' 생성 
	FMatrix VP = ViewProjConstants.View * ViewProjConstants.Projection;
	CurrentFrustum.Planes[0] = VP[3] + VP[0]; // Left
	CurrentFrustum.Planes[1] = VP[3] - VP[0]; // Right
	CurrentFrustum.Planes[2] = VP[3] + VP[1]; // Bottom
	CurrentFrustum.Planes[3] = VP[3] - VP[1]; // Top
	CurrentFrustum.Planes[4] = VP[3] + VP[2]; // Near
	CurrentFrustum.Planes[5] = VP[3] - VP[2]; // Far

	for (int i = 0; i < 6; i++)
	{
		const float Length = sqrt((CurrentFrustum.Planes[i].X * CurrentFrustum.Planes[i].X) +
								(CurrentFrustum.Planes[i].Y * CurrentFrustum.Planes[i].Y) +
								(CurrentFrustum.Planes[i].Z * CurrentFrustum.Planes[i].Z));

		if (Length > -MATH_EPSILON && Length < MATH_EPSILON) { return; }

		CurrentFrustum.Planes[i] /= -Length;
	}

	// 2. 옥트리를 이용해 보이는 객체만 RenderableObjects에 저장한다.
	if (StaticOctree)
	{
		CullOctree(StaticOctree);
	}

	if (DynamicOctree)
	{
		CullOctree(DynamicOctree);
	}
}

const TArray<TObjectPtr<UPrimitiveComponent>>& ViewVolumeCuller::GetRenderableObjects() const
{
	return RenderableObjects;
}

void ViewVolumeCuller::CullOctree(FOctree* Octree)
{
	if (!Octree)
	{
		return;
	}

	// 1. 현재 옥트리 노드(자신)의 경계와 절두체의 관계를 확인합니다.
	EBoundCheckResult result = CurrentFrustum.CheckIntersection(Octree->GetBoundingBox());

	// Case 1. 노드가 절두체 밖에 있다면, 즉시 종료합니다. 
	if (result == EBoundCheckResult::Outside)
	{
		return;
	}
	// Case 2. 노드가 절두체 안에 완전히 포함된다면, 전부 포함하고 종료합니다.
	else if (result == EBoundCheckResult::Inside)
	{
		TArray<UPrimitiveComponent*> Primitives;
		Octree->GetAllPrimitives(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			RenderableObjects.push_back(TObjectPtr<UPrimitiveComponent>(Primitive));
		}
		return;
	}
	// Case 3. 노드가 절두체와 부분적으로 겹쳐진다면, 개별 검사를 합니다.
	else if (result == EBoundCheckResult::Intersect)
	{
		for (UPrimitiveComponent* Primitive : Octree->GetPrimitives())
		{
			if (Primitive)
			{
				if (CurrentFrustum.CheckIntersection(GetPrimitiveBoundingBox(Primitive)) != EBoundCheckResult::Outside)
				{
					RenderableObjects.push_back(TObjectPtr<UPrimitiveComponent>(Primitive));
				}
			}
		}

		// 2. 자식 노드들에게 재귀적으로 검사를 계속 진행시킵니다.
		if (!Octree->IsLeafNode())
		{
			for (int Index = 0; Index < 8; ++Index)
			{
				if (Octree->GetChildren()[Index])
				{
					CullOctree(Octree->GetChildren()[Index]);
				}
			}
		}
	}
}