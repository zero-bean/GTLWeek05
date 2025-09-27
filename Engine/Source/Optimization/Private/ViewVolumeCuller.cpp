#include "pch.h"
#include "Optimization/Public/ViewVolumeCuller.h"
#include "Core/Public/Object.h"
#include "Global/Octree.h"

void ViewVolumeCuller::Cull(FOctree* StaticOctree, FOctree* DynamicOctree, const FViewProjConstants& ViewProjConstants)
{
	// 이전의 Cull했던 정보를 지운다.
	RenderableObjects.clear();
	CurrentFrustum.Clear();

	// 1. 절두체 'Key' 생성 
	FFrustum CurrentFrustum;
	FMatrix VP = ViewProjConstants.View * ViewProjConstants.Projection;
	CurrentFrustum.Planes[0] = VP[3] + VP[0]; // Left
	CurrentFrustum.Planes[1] = VP[3] - VP[0]; // Right
	CurrentFrustum.Planes[2] = VP[3] + VP[1]; // Bottom
	CurrentFrustum.Planes[3] = VP[3] - VP[1]; // Top
	CurrentFrustum.Planes[4] = VP[2];           // Near
	CurrentFrustum.Planes[5] = VP[3] - VP[2]; // Far
	for (int i = 0; i < 6; i++) { CurrentFrustum.Planes[i].Normalize(); }

	// 2. 옥트리를 이용해 보이는 객체만 RenderableObjects에 저장한다.
	TArray<UPrimitiveComponent*>& TempArray = reinterpret_cast<TArray<UPrimitiveComponent*>&>(RenderableObjects);
	if (StaticOctree)
	{
		StaticOctree->FindVisiblePrimitives(CurrentFrustum, TempArray);
	}
	if (DynamicOctree)
	{
		DynamicOctree->FindVisiblePrimitives(CurrentFrustum, TempArray);
	}
}

const TArray<TObjectPtr<UPrimitiveComponent>>& ViewVolumeCuller::GetRenderableObjects() const
{
	return RenderableObjects;
}