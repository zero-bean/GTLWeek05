﻿#pragma once
#include "Component/Public/SceneComponent.h"
#include "Physics/Public/BoundingVolume.h"

UCLASS()
class UPrimitiveComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

public:
	UPrimitiveComponent();

	void TickComponent() override;

	virtual void OnSelected() override;
	virtual void OnDeselected() override;

	const TArray<FNormalVertex>* GetVerticesData() const;
	const TArray<uint32>* GetIndicesData() const;
	ID3D11Buffer* GetVertexBuffer() const;
	ID3D11Buffer* GetIndexBuffer() const;
	uint32 GetNumVertices() const;
	uint32 GetNumIndices() const;

	FRenderState& GetRenderState() { return RenderState; }

	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology);
	D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;
	//void Render(const URenderer& Renderer) const override;

	bool IsVisible() const { return bVisible; }
	void SetVisibility(bool bVisibility) { bVisible = bVisibility; }

	FVector4 GetColor() const { return Color; }
	void SetColor(const FVector4& InColor) { Color = InColor; }

	const IBoundingVolume* GetBoundingBox() const { return BoundingBox; }
	void GetWorldAABB(FVector& OutMin, FVector& OutMax) const;

	EPrimitiveType GetPrimitiveType() const { return Type; }

	virtual void MarkAsDirty() override;

	// 다른 곳에서 사용할 인덱스
	mutable int32 CachedAABBIndex = -1;
	mutable uint32 CachedFrame = 0;

protected:
	const TArray<FNormalVertex>* Vertices = nullptr;
	const TArray<uint32>* Indices = nullptr;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	uint32 NumVertices = 0;
	uint32 NumIndices = 0;

	FVector4 Color = FVector4{ 0.f,0.f,0.f,0.f };

	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mutable FRenderState RenderState = {};
	EPrimitiveType Type = EPrimitiveType::Cube;

	bool bVisible = true;

	const IBoundingVolume* BoundingBox = nullptr;
	
	mutable FVector CachedWorldMin;
	mutable FVector CachedWorldMax;
	mutable bool bIsAABBCacheDirty = true;

public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;
};
