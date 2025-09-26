#include "pch.h"
#include "Optimization/Public/PerspectiveFrustumCuller.h"

void UPerspectiveFrustumCuller::Cull(
	const TArray<TObjectPtr<UPrimitiveComponent>>& Objects,
	const FViewProjConstants& ViewProjConstants
)
{
	FMatrix VP = ViewProjConstants.View * ViewProjConstants.Projection;

	for (const TObjectPtr<UPrimitiveComponent>& Object : Objects)
	{
		FMatrix MVP = Object->GetWorldTransformMatrix() * VP;

		// get 4 columns of MVP
		FVector4 AddRow4Row1 = MVP[3] + MVP[0];
		FVector4 SubRow4Row1 = MVP[3] - MVP[0];
		FVector4 AddRow4Row2 = MVP[3] + MVP[1];
		FVector4 SubRow4Row2 = MVP[3] - MVP[1];
		FVector4 AddRow4Row3 = MVP[3] + MVP[2];
		FVector4 SubRow4Row3 = MVP[3] - MVP[2];

		// define normals of 6 frustum planes
		double Denomiator[6] = {};

		Denomiator[0] = -sqrt(pow(AddRow4Row1.X, 2.0) + pow(AddRow4Row1.Y, 2.0) + pow(AddRow4Row1.Z, 2.0));
		Denomiator[1] = -sqrt(pow(SubRow4Row1.X, 2.0) + pow(SubRow4Row1.Y, 2.0) + pow(SubRow4Row1.Z, 2.0));
		Denomiator[2] = -sqrt(pow(AddRow4Row2.X, 2.0) + pow(AddRow4Row2.Y, 2.0) + pow(AddRow4Row2.Z, 2.0));
		Denomiator[3] = -sqrt(pow(SubRow4Row2.X, 2.0) + pow(SubRow4Row2.Y, 2.0) + pow(SubRow4Row2.Z, 2.0));
		Denomiator[4] = -sqrt(pow(AddRow4Row3.X, 2.0) + pow(AddRow4Row3.Y, 2.0) + pow(AddRow4Row3.Z, 2.0));
		Denomiator[5] = -sqrt(pow(AddRow4Row3.X, 2.0) + pow(AddRow4Row3.Y, 2.0) + pow(AddRow4Row3.Z, 2.0));

		// divide with zero 방지
		for (int i = 0; i < 6; i++)
		{
			// culling하지 않고 모든 오브젝트를 그린다.
			if (Denomiator[i] >= -0.0001f || Denomiator[i] <= 0.0001f)
				RenderableObjects = Objects;
		}

		FVector4 Normal[6] = {};

		Normal[0] = AddRow4Row1 / Denomiator[0];
		Normal[1] = SubRow4Row1 / Denomiator[1];
		Normal[2] = AddRow4Row2 / Denomiator[2];
		Normal[3] = SubRow4Row2 / Denomiator[3];
		Normal[4] = AddRow4Row3 / Denomiator[4];
		Normal[5] = SubRow4Row3 / Denomiator[5];

		const FAABB* AABB = dynamic_cast<const FAABB*>(Object->GetBoundingBox());

		FVector4 Corner[8] = {};

		// define 8 corners of AABB
		Corner[0] = { FVector4(AABB->Min.X, AABB->Max.Y, AABB->Min.Z, 1.0f) };
		Corner[1] = { FVector4(AABB->Min.X, AABB->Max.Y, AABB->Min.Z, 1.0f) };
		Corner[2] = { FVector4(AABB->Max.X, AABB->Min.Y, AABB->Min.Z, 1.0f) };
		Corner[3] = { FVector4(AABB->Max.X, AABB->Max.Y, AABB->Min.Z, 1.0f) };
		Corner[4] = { FVector4(AABB->Max.X, AABB->Min.Y, AABB->Max.Z, 1.0f) };
		Corner[5] = { FVector4(AABB->Min.X, AABB->Max.Y, AABB->Max.Z, 1.0f) };
		Corner[6] = { FVector4(AABB->Min.X, AABB->Min.Y, AABB->Min.Z, 1.0f) };
		Corner[7] = { FVector4(AABB->Max.X, AABB->Max.Y, AABB->Max.Z, 1.0f) };

		bool NotOutside = false;
		bool NotInside = false;
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				if (Normal[j].Dot3(Corner[i]) + Normal[j].W < 0.0f)
					NotOutside = true;
				else
					NotInside = true;
			}
		}

		EBoundCheckResult BoundCheckResult;

		if (NotOutside && !NotInside)
			BoundCheckResult = EBoundCheckResult::Inside;
		else if (NotOutside && NotInside)
			BoundCheckResult = EBoundCheckResult::Intersect;
		else
			BoundCheckResult = EBoundCheckResult::Outside;

		if (BoundCheckResult == EBoundCheckResult::Intersect ||
			BoundCheckResult == EBoundCheckResult::Inside)
			RenderableObjects.push_back(Object);
	}

	Total = Objects.size();
	Rendered = RenderableObjects.size();
	Culled = Total - Rendered;
}