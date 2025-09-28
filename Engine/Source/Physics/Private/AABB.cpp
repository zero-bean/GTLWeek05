#include "pch.h"
#include "Physics/Public/AABB.h"

float FAABB::GetCenterDistanceSquared(const FVector& Point) const
{
    FVector Center = GetCenter();
    FVector Diff = Center - Point;

    return (Diff.X * Diff.X) + (Diff.Y * Diff.Y) + (Diff.Z * Diff.Z);
}

bool FAABB::IsContains(const FAABB& Other) const
{
    return (Other.Min.X >= Min.X && Other.Max.X <= Max.X) &&
        (Other.Min.Y >= Min.Y && Other.Max.Y <= Max.Y) &&
        (Other.Min.Z >= Min.Z && Other.Max.Z <= Max.Z);
}

bool FAABB::IsIntersected(const FAABB& Other) const
{
	return (Min.X <= Other.Max.X && Max.X >= Other.Min.X) &&
		(Min.Y <= Other.Max.Y && Max.Y >= Other.Min.Y) &&
		(Min.Z <= Other.Max.Z && Max.Z >= Other.Min.Z);
}

bool FAABB::RaycastHit() const
{
	return false;
}

bool CheckIntersectionRayBox(const FRay& Ray, const FAABB& Box)
{
	// AABB intersectin test by "Slab Method"
    float TMin = -FLT_MAX;
    float TMax = FLT_MAX;

    for (int Axis = 0; Axis < 3; ++Axis)
    {
        float Origin = (Axis == 0) ? Ray.Origin.X : (Axis == 1) ? Ray.Origin.Y : Ray.Origin.Z;
        float Dir = (Axis == 0) ? Ray.Direction.X : (Axis == 1) ? Ray.Direction.Y : Ray.Direction.Z;
        float BMin = (Axis == 0) ? Box.Min.X : (Axis == 1) ? Box.Min.Y : Box.Min.Z;
        float BMax = (Axis == 0) ? Box.Max.X : (Axis == 1) ? Box.Max.Y : Box.Max.Z;

		// if the ray is parallel to the slab
        if (fabs(Dir) < MATH_EPSILON)
        {
            if (Origin < BMin || Origin > BMax)
            {
				return false; // ray is parallel and outside the slab
            }
            continue;
        }

        float T1 = (BMin - Origin) / Dir;
        float T2 = (BMax - Origin) / Dir;
        if (T1 > T2) std::swap(T1, T2);

        TMin = std::max(TMin, T1);
        TMax = std::min(TMax, T2);
        if (TMax < TMin) return false;
    }

	if (TMax < 0.0f) return false; // box is behind the ray

    return true;
}