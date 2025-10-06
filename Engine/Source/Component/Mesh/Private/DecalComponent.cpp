#include "pch.h"
#include "Component/Mesh/Public/DecalComponent.h"
#include "Texture/Public/Material.h" 
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UDecalComponent, UPrimitiveComponent)

UDecalComponent::UDecalComponent() : DecalMaterial(nullptr)
{
	if (DecalMaterial = NewObject<UMaterial>())
	{
		UAssetManager& AssetManager = UAssetManager::GetInstance();
		UTexture* DiffuseTexture = AssetManager.CreateTexture(FName("Asset/Texture/maple.jpg"), FName("MapleDiffuse"));
		DecalMaterial->SetDiffuseTexture(DiffuseTexture);
	}
}

UDecalComponent::~UDecalComponent()
{
	
}

void UDecalComponent::SetDecalMaterial(UMaterial* InMaterial)
{
	DecalMaterial = InMaterial;
}

UMaterial* UDecalComponent::GetDecalMaterial() const
{
	return DecalMaterial;
}

UObject* UDecalComponent::Duplicate()
{
	UDecalComponent* DecalComponent = Cast<UDecalComponent>(Super::Duplicate());

	return DecalComponent;
}

void UDecalComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}