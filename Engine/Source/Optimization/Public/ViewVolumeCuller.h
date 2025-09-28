#pragma once

#include "Component/Public/PrimitiveComponent.h"
#include "Optimization/Public/BSP.h"

enum class EPlaneIntersection
{
	Outside,
	Intersect,
	Inside
};

class FViewVolumeCuller
{
public:
	FViewVolumeCuller() = default;
	~FViewVolumeCuller() = default;
	FViewVolumeCuller(const FViewVolumeCuller& Other) = default;
	FViewVolumeCuller& operator=(const FViewVolumeCuller& Other) = default;

	void Cull(
		const TArray<TObjectPtr<UPrimitiveComponent>>& Objects,
		const FViewProjConstants& ViewProjConstants
	);


	void Cull(
		FBSP& BSP,
		const FViewProjConstants& ViewProjConstants
	);

	void Clear();

	const TArray<TObjectPtr<UPrimitiveComponent>>& GetRenderableObjects() const;

private:
	static void GetCullingCandidatesFromBSP(
		FBSP& BSP,
		const FViewProjConstants& ViewProjConstants,
		TArray<TObjectPtr<UPrimitiveComponent>>& Primitives
	);

	uint32 Total = 0;
	uint32 Rendered = 0;
	uint32 Culled = 0;

	TArray<TObjectPtr<UPrimitiveComponent>> RenderableObjects;
};