#pragma once

#include "Physics/Public/AABB.h"
#include "Optimization/Public/FrustumCuller.h"

enum class EBoundCheckResult
{
	Outside,
	Intersect,
	Inside
};

class UPerspectiveFrustumCuller : public IFrustumCuller
{
public:
	void Cull(
		const TArray<TObjectPtr<UPrimitiveComponent>>& Objects,
		const FViewProjConstants& ViewProjConstants
	) override;
private:
};