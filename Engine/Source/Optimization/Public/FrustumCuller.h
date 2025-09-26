#pragma once

#include "Component/Public/PrimitiveComponent.h"

class IFrustumCuller
{
public:
	IFrustumCuller() = default;
	~IFrustumCuller() = default;
	IFrustumCuller(const IFrustumCuller& Other) = default;
	IFrustumCuller& operator=(const IFrustumCuller& Other) = default;

	virtual void Cull(
		const TArray<TObjectPtr<UPrimitiveComponent>>& Objects,
		const FViewProjConstants& ViewProjConstants
	) = 0;

	TArray<TObjectPtr<UPrimitiveComponent>>& GetRenderableObjects()
	{
		return RenderableObjects;
	}
protected:
	uint32 Total = 0;
	uint32 Rendered = 0;
	uint32 Culled = 0;

	TArray<TObjectPtr<UPrimitiveComponent>> RenderableObjects;
};