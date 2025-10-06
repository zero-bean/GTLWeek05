#pragma once

enum class EBoundingVolumeType
{
	None,
	AABB,   // Axis-Aligned Bounding Box
	OBB,
	Sphere  // Bounding Sphere
};

class IBoundingVolume
{
public:
	virtual ~IBoundingVolume() = default;
	virtual bool RaycastHit() const = 0;
	virtual EBoundingVolumeType GetType() const = 0;
	virtual bool Intersects(const IBoundingVolume& Other) const = 0;
};
