#include "pch.h"
#include "Physics/Public/AABB.h"
#include "Optimization/Public/ViewVolumeCuller.h"
#include "Core/Public/Object.h"

void FViewVolumeCuller::Cull(
	const TArray<TObjectPtr<UPrimitiveComponent>>& Objects,
	const FViewProjConstants& ViewProjConstants
)
{

	// ������ Cull�ߴ� ������ �����.
	RenderableObjects.clear();

	FMatrix VP = ViewProjConstants.View * ViewProjConstants.Projection;

	Objects.size();

	for (const TObjectPtr<UPrimitiveComponent>& Object : Objects)
	{
		FMatrix MVP = Object->GetWorldTransformMatrix() * VP;

		// 6���� ����ü ��� ����
		FVector4 Plane[6];
		Plane[0] = MVP[3] + MVP[0];  // Left
		Plane[1] = MVP[3] - MVP[0];  // Right
		Plane[2] = MVP[3] + MVP[1];  // Bottom
		Plane[3] = MVP[3] - MVP[1];  // Top
		Plane[4] = MVP[2];			 // Near
		Plane[5] = MVP[3] - MVP[2];  // Far

		for (int32 i = 0; i < 6; i++)
		{
			float length = sqrt(
				Plane[i].X * Plane[i].X +
				Plane[i].Y * Plane[i].Y +
				Plane[i].Z * Plane[i].Z
			);

			// Divide with zero ����
			if (length > -0.0001f && length < 0.0001f)
			{
				RenderableObjects = Objects;
				Total = Objects.size();
				Rendered = Total;
				Culled = 0;
				return;
			}

			Plane[i] /= -length;
		}

		if (!Object->GetBoundingBox())
		{
			RenderableObjects.push_back(Object);
			continue;
		}

		const FAABB* AABB = static_cast<const FAABB*>(Object->GetBoundingBox());

		EBoundCheckResult BoundCheckResult = EBoundCheckResult::Inside;

		// �ڽ��� ������ ��鿡 ���� ����� ��, ���� �� ���� ���Ѵ�.
		for (int32 i = 0; i < 6; i++)
		{
			FVector Closest, Farthest;

			if (Plane[i].X > 0.0f)
			{
				Closest.X = AABB->Min.X;
				Farthest.X = AABB->Max.X;
			}
			else
			{
				Closest.X = AABB->Max.X;
				Farthest.X = AABB->Min.X;
			}

			if (Plane[i].Y > 0.0f)
			{
				Closest.Y = AABB->Min.Y;
				Farthest.Y = AABB->Max.Y;
			}
			else
			{
				Closest.Y = AABB->Max.Y;
				Farthest.Y = AABB->Min.Y;
			}

			if (Plane[i].Z > 0.0f)
			{
				Closest.Z = AABB->Min.Z;
				Farthest.Z = AABB->Max.Z;
			}
			else
			{
				Closest.Z = AABB->Max.Z;
				Farthest.Z = AABB->Min.Z;
			}

			if (Plane[i].Dot3(Closest) + Plane[i].W > 0.0f)
			{
				BoundCheckResult = EBoundCheckResult::Outside;
				break;
			}
			else if (Plane[i].Dot3(Farthest) + Plane[i].W < 0.0f)
				;
			else
				BoundCheckResult = EBoundCheckResult::Intersect;
		}

		if (BoundCheckResult != EBoundCheckResult::Outside)
			RenderableObjects.push_back(Object);
	}

	Total = Objects.size();
	Rendered = RenderableObjects.size();
	Culled = Total - Rendered;
}

const TArray<TObjectPtr<UPrimitiveComponent>>& FViewVolumeCuller::GetRenderableObjects() const
{
	return RenderableObjects;
}