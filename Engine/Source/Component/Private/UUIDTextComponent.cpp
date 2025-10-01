#include "pch.h"
#include "Component/Public/UUIDTextComponent.h"
#include "Editor/Public/Editor.h"
#include "Actor/Public/Actor.h"

IMPLEMENT_CLASS(UUUIDTextComponent, UTextComponent)

/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */

UUUIDTextComponent::UUUIDTextComponent() : POwnerActor(nullptr), ZOffset(0.0f)
{
	Type = EPrimitiveType::UUID;
};

UUUIDTextComponent::UUUIDTextComponent(AActor* InOwnerActor, float InYOffset)
	: POwnerActor(InOwnerActor)
	, ZOffset(InYOffset)
{
	Type = EPrimitiveType::UUID;
	SetVisibility(false); // 현재는 시작하자마자 Visibility False, Select 시 True되는 시스템
}

UUUIDTextComponent::~UUUIDTextComponent()
{
}

void UUUIDTextComponent::OnSelected()
{
	SetVisibility(true);
}

void UUUIDTextComponent::OnDeselected()
{
	SetVisibility(false);
}

void UUUIDTextComponent::UpdateRotationMatrix(const FVector& InCameraLocation)
{
	const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();

	FVector ToCamera = InCameraLocation - OwnerActorLocation;
	ToCamera.Normalize();

	const FVector4 worldUp4 = FVector4(0, 0, 1, 1);
	const FVector worldUp = { worldUp4.X, worldUp4.Y, worldUp4.Z };
	FVector Right = worldUp.Cross(ToCamera);
	Right.Normalize();
	FVector Up = ToCamera.Cross(Right);
	Up.Normalize();

	RTMatrix = FMatrix(FVector4(0, 1, 0, 1), worldUp4, FVector4(1,0,0,1));
	RTMatrix = FMatrix(ToCamera, Right, Up);
	//RTMatrix = FMatrix::Identity();
	//UE_LOG("%.2f, %.2f, %.2f", ToCamera.X, ToCamera.Y, ToCamera.Z);

	const FVector Translation = OwnerActorLocation + FVector(0.0f, 0.0f, ZOffset);
	//UE_LOG("%.2f, %.2f, %.2f", Translation.X, Translation.Y, Translation.Z);
	RTMatrix *= FMatrix::TranslationMatrix(Translation);
}

TObjectPtr<UClass> UUUIDTextComponent::GetSpecificWidgetClass() const
{
	return TObjectPtr<UClass>();
}
