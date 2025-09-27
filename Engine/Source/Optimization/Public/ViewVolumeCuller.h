#pragma once

#include "Component/Public/PrimitiveComponent.h"
#include "Physics/Public/AABB.h"

enum class EBoundCheckResult
{
	Outside,
	Intersect,
	Inside
};

struct FFrustum
{
    FVector4 Planes[6];

    EBoundCheckResult CheckIntersection(const FAABB& BBox) const
    {
        EBoundCheckResult Result = EBoundCheckResult::Inside;

        for (int i = 0; i < 6; ++i)
        {
            const FVector4& P = Planes[i];
            FVector PVertex(
                P.X > 0 ? BBox.Max.X : BBox.Min.X,
                P.Y > 0 ? BBox.Max.Y : BBox.Min.Y,
                P.Z > 0 ? BBox.Max.Z : BBox.Min.Z
            );
            FVector NVertex(
                P.X > 0 ? BBox.Min.X : BBox.Max.X,
                P.Y > 0 ? BBox.Min.Y : BBox.Max.Y,
                P.Z > 0 ? BBox.Min.Z : BBox.Max.Z
            );

            if (P.Dot3(NVertex) + P.W > 0)
            {
                return EBoundCheckResult::Outside;
            }
            if (P.Dot3(PVertex) + P.W > 0)
            {
                Result = EBoundCheckResult::Intersect;
            }
        }
        return Result;
    }
};

class ViewVolumeCuller
{
public:
	ViewVolumeCuller() = default;
	~ViewVolumeCuller() = default;
	ViewVolumeCuller(const ViewVolumeCuller& Other) = default;
	ViewVolumeCuller& operator=(const ViewVolumeCuller& Other) = default;

	void Cull(
		const TArray<TObjectPtr<UPrimitiveComponent>>& Objects,
		const FViewProjConstants& ViewProjConstants
	);

	const TArray<TObjectPtr<UPrimitiveComponent>>& GetRenderableObjects() const;
private:
	uint32 Total = 0;
	uint32 Rendered = 0;
	uint32 Culled = 0;

	TArray<TObjectPtr<UPrimitiveComponent>> RenderableObjects;
};