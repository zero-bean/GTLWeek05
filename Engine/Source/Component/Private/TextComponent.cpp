#include "pch.h"
#include "Component/Public/TextComponent.h"

#include "Editor/Public/Editor.h"
#include "Actor/Public/Actor.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Render/UI/Widget/Public/SetTextComponentWidget.h"

IMPLEMENT_CLASS(UTextComponent, UPrimitiveComponent)

/**
 * @brief Level���� �� Actor���� ������ �ִ� UUID�� ������ֱ� ���� ������ Ŭ����
 * Actor has a UBillBoardComponent
 */
UTextComponent::UTextComponent()
{
	Type = EPrimitiveType::Text;

	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
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

UObject* UTextComponent::Duplicate()
{
	UTextComponent* TextComponent = Cast<UTextComponent>(Super::Duplicate());
	TextComponent->Text = Text;
	return TextComponent;
}

void UTextComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	UPrimitiveComponent::DuplicateSubObjects(DuplicatedObject);
}
