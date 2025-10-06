#include "pch.h"
#include "Component/Mesh/Public/CubeComponent.h"

#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"

IMPLEMENT_CLASS(UCubeComponent, UPrimitiveComponent)

UCubeComponent::UCubeComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Cube;

	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}

void UCubeComponent::UseLineRendering(bool bUseLine)
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	if (bUseLine)
	{
		Type = EPrimitiveType::CubeLine;
		Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	}
	else
	{
		Type = EPrimitiveType::Cube;
		Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	Vertices = ResourceManager.GetVertexData(Type);
	VertexBuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);

	Indices = ResourceManager.GetIndexData(Type);
	IndexBuffer = ResourceManager.GetIndexbuffer(Type);
	NumIndices = ResourceManager.GetNumIndices(Type);

	BoundingBox = &ResourceManager.GetAABB(Type);
}
