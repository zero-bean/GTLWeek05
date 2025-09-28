#pragma once
#include "Physics/Public/BoundingVolume.h"
#include "Global/Vector.h"

struct FAABB : public IBoundingVolume
{
	FVector Min;
	FVector Max;

	FAABB() : Min(0.f, 0.f, 0.f), Max(0.f, 0.f, 0.f) {}
	FAABB(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}

	FVector GetCenter() const { return (Min + Max) * 0.5f; }

	float GetCenterDistanceSquared(const FVector& Point) const;

	bool IsContains(const FAABB& Other) const;

	bool IsIntersected(const FAABB& Other) const;

	bool RaycastHit() const override;

	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::AABB; }
};
