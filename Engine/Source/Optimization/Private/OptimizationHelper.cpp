#include "pch.h"
#include "Optimization/Public/OptimizationHelper.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Viewport.h"

/* Primitive의 추가를 알아야 하는 다른 객체들에게 알린다. */
void Optimization::NotifyPrimitiveAdditionToOthers()
{
	URenderer::GetInstance(). \
		GetViewportClient()->NotifyViewVolumeCullerDirtyToClientCamera();
}

/* Primitive의 삭제를 알아야 하는 다른 객체들에게 알린다. */
void Optimization::NotifyPrimitiveDeletionToOthers()
{
	URenderer::GetInstance(). \
		GetViewportClient()->NotifyViewVolumeCullerDirtyToClientCamera();
}

/* Primitive의 변경 사항을 알아야 하는 다른 객체들에게 알린다. */
void Optimization::NotifyPrimitiveDirtyToOthers()
{
	URenderer::GetInstance(). \
		GetViewportClient()->NotifyViewVolumeCullerDirtyToClientCamera();
}