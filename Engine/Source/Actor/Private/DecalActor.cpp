#include "pch.h"
#include "Actor/Public/DecalActor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Mesh/Public/DecalComponent.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Texture/Public/Material.h"

IMPLEMENT_CLASS(ADecalActor, AActor)

ADecalActor::ADecalActor()
{
    VisualizationComponent = CreateDefaultSubobject<UCubeComponent>(FName("DecalVisualComponent"));
    VisualizationComponent->GetRenderState().FillMode = EFillMode::WireFrame;
    VisualizationComponent->GetRenderState().CullMode = ECullMode::None;
    VisualizationComponent->SetRelativeLocation(FVector::ZeroVector());
    VisualizationComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
    VisualizationComponent->UseLineRendering(true);
}

ADecalActor::~ADecalActor()
{

}

void ADecalActor::InitializeComponents()
{
    Super::InitializeComponents();

    if (VisualizationComponent)
    {
        VisualizationComponent->SetParentAttachment(GetRootComponent());
    }
}

UClass* ADecalActor::GetDefaultRootComponent()
{
    return UDecalComponent::StaticClass();
}
