#include "pch.h"
#include "Component/Public/TextComponent.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Editor/Public/Editor.h"
#include "Actor/Public/Actor.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Render/UI/Widget/Public/SetTextComponentWidget.h"

IMPLEMENT_CLASS(UTextComponent, UPrimitiveComponent)

/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */
UTextComponent::UTextComponent()
{
	Type = EPrimitiveType::Text;

	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;

	SetRelativeRotation({ 0.0f, -90.0f, -90.0f });
}

UTextComponent::~UTextComponent()
{
}

void UTextComponent::UpdateRotationMatrix(const FVector& InCameraLocation) {}
FMatrix UTextComponent::GetRTMatrix() const { return FMatrix(); }

const FString& UTextComponent::GetText() { return Text; }
void UTextComponent::SetText(const FString& InText) { Text = InText; }

TObjectPtr<UClass> UTextComponent::GetSpecificWidgetClass() const
{
	return USetTextComponentWidget::StaticClass();
}