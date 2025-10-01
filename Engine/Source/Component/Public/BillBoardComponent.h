#pragma once

#include "Component/Public/PrimitiveComponent.h"
#include "Render/UI/Widget/Public/SpriteSelectionWidget.h"

UCLASS()
class UBillBoardComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillBoardComponent, UPrimitiveComponent)

public:
	UBillBoardComponent();
	~UBillBoardComponent();

	void FaceCamera(
		const FVector& CameraPosition,
		const FVector& CameraUp,
		const FVector& FallbackUp
	);

	const TPair<FName, ID3D11ShaderResourceView*>& GetSprite() const;
	void SetSprite(const TPair<FName, ID3D11ShaderResourceView*>& Sprite);

	const ID3D11SamplerState* GetSampler() const;

	/**
 * @brief 특정 컴포넌트 전용 Widget이 필요할 경우 재정의 필요
 */
	TObjectPtr<UClass> GetSpecificWidgetClass() const override
	{
		return USpriteSelectionWidget::StaticClass();
	}

private:
	TPair<FName, ID3D11ShaderResourceView*> Sprite = {"None", nullptr};
	ID3D11SamplerState* Sampler = nullptr;
};