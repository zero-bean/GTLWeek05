#pragma once

#include "Physics/Public/AABB.h"

enum class EBoundCheckResult
{
	Outside,
	Intersect,
	Inside
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

	TArray<TObjectPtr<UPrimitiveComponent>>& GetRenderableObjects();
private:
	uint32 Total = 0;
	uint32 Rendered = 0;
	uint32 Culled = 0;

	TArray<TObjectPtr<UPrimitiveComponent>> RenderableObjects;
};