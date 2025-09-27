#pragma once

#include "Component/Public/PrimitiveComponent.h"

enum class EBoundCheckResult
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

	const TArray<TObjectPtr<UPrimitiveComponent>>& GetRenderableObjects() const;
private:
	uint32 Total = 0;
	uint32 Rendered = 0;
	uint32 Culled = 0;

	TArray<TObjectPtr<UPrimitiveComponent>> RenderableObjects;
};