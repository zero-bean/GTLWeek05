#include "pch.h"
#include "Physics/Public/AABB.h"
#include "Optimization/Public/ViewVolumeCuller.h"
#include "Core/Public/Object.h"

void FViewVolumeCuller::Cull(
	const TArray<TObjectPtr<UPrimitiveComponent>>& Objects,
	const FViewProjConstants& ViewProjConstants
)
{
	// 이전의 Cull했던 정보를 지운다.
	Clear();

	FMatrix VP = ViewProjConstants.View * ViewProjConstants.Projection;

	for (const TObjectPtr<UPrimitiveComponent>& Object : Objects)
	{
		FMatrix MVP = Object->GetWorldTransformMatrix() * VP;

		// 6개의 절두체 평면 추출
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

			// Divide with zero 방지
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

		const FAABB* AABB = static_cast<const FAABB*>(Object->GetBoundingBox());
		if (!AABB)
		{
			RenderableObjects.push_back(Object);
			continue;
		}


		EPlaneIntersection BoundCheckResult = EPlaneIntersection::Inside;

		// 박스의 점들중 평면에 가장 가까운 점, 가장 먼 점만 비교한다.
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
				BoundCheckResult = EPlaneIntersection::Outside;
				break;
			}
			else if (Plane[i].Dot3(Farthest) + Plane[i].W < 0.0f)
				;
			else
				BoundCheckResult = EPlaneIntersection::Intersect;
		}

		if (BoundCheckResult != EPlaneIntersection::Outside)
			RenderableObjects.push_back(Object);
	}

	Total = Objects.size();
	Rendered = RenderableObjects.size();
	Culled = Total - Rendered;
}

void FViewVolumeCuller::Cull(
	FBSP& BSP,
	const FViewProjConstants& ViewProjConstants
)
{
	TArray<TObjectPtr<UPrimitiveComponent>> Primitives;
	GetCullingCandidatesFromBSP(BSP, ViewProjConstants, Primitives);

	Cull(Primitives, ViewProjConstants);
}

void FViewVolumeCuller::Clear()
{
	RenderableObjects.clear();

	Total = 0;
	Rendered = 0;
	Culled = 0;
}

const TArray<TObjectPtr<UPrimitiveComponent>>& FViewVolumeCuller::GetRenderableObjects() const
{
	return RenderableObjects;
}

void FViewVolumeCuller::GetCullingCandidatesFromBSP(
	FBSP& BSP,
	const FViewProjConstants& ViewProjConstants,
	TArray<TObjectPtr<UPrimitiveComponent>>& Primitives
)
{
	FMatrix VP = ViewProjConstants.View * ViewProjConstants.Projection;

	FVector4 Plane[6];

	Plane[0] = VP[3] + VP[0];
	Plane[1] = VP[3] - VP[0];
	Plane[2] = VP[3] + VP[1];
	Plane[3] = VP[3] - VP[1];
	Plane[5] = VP[3];
	Plane[4] = VP[3] - VP[2];
	
	for (int i = 0; i < 6; i++)
	{
		float length = sqrt(
			Plane[i].X * Plane[i].X +
			Plane[i].Y * Plane[i].Y +
			Plane[i].Z * Plane[i].Z
		);

		// Divide with zero 방지
		if (length > -0.0001f && length < 0.0001f)
			return;

		Plane[i] /= -length;
	}

	FBSP::PreOrderUntil(
		BSP.GetRoot(),
		[Plane, &Primitives](BSPNode* Node) -> bool
		{
			EPlaneIntersection BoundCheckResult = EPlaneIntersection::Inside;

			// 박스의 점들중 평면에 가장 가까운 점, 가장 먼 점만 비교한다.
			for (int32 i = 0; i < 6; i++)
			{
				FVector Closest, Farthest;

				if (Plane[i].X > 0.0f)
				{
					Closest.X = Node->Position.X - Node->Extent.X / 2.0f;
					Farthest.X = Node->Position.X + Node->Extent.X / 2.0f;
				}
				else
				{
					Closest.X = Node->Position.X + Node->Extent.X / 2.0f;
					Farthest.X = Node->Position.X - Node->Extent.X / 2.0f;
				}

				if (Plane[i].Y > 0.0f)
				{
					Closest.Y = Node->Position.Y - Node->Extent.Y / 2.0f;
					Farthest.Y = Node->Position.Y + Node->Extent.Y / 2.0f;
				}
				else
				{
					Closest.Y = Node->Position.Y + Node->Extent.Y / 2.0f;
					Farthest.Y = Node->Position.Y - Node->Extent.Y / 2.0f;
				}

				if (Plane[i].Z > 0.0f)
				{
					Closest.Z = Node->Position.Z - Node->Extent.Z / 2.0f;
					Farthest.Z = Node->Position.Z + Node->Extent.Z / 2.0f;
				}
				else
				{
					Closest.Z = Node->Position.Z + Node->Extent.Z / 2.0f;
					Farthest.Z = Node->Position.Z - Node->Extent.Z / 2.0f;
				}

				if (Plane[i].Dot3(Closest) + Plane[i].W > 0.0f)
				{
					BoundCheckResult = EPlaneIntersection::Outside;
					break;
				}
				else if (Plane[i].Dot3(Farthest) + Plane[i].W < 0.0f)
					;
				else
					BoundCheckResult = EPlaneIntersection::Intersect;
			}

			if (BoundCheckResult != EPlaneIntersection::Outside)
			{
				for (const TObjectPtr<UPrimitiveComponent> &Primitive : Node->Primitives)
					Primitives.push_back(Primitive);
				return true;
			}

			return false;
		}
	);
}