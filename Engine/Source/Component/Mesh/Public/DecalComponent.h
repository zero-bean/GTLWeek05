#pragma once
#include "Component/Public/PrimitiveComponent.h"

class UMaterial;

UCLASS()
class UDecalComponent : public UPrimitiveComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

public:
    UDecalComponent();
    virtual ~UDecalComponent();

    void SetDecalMaterial(UMaterial* InMaterial);
    UMaterial* GetDecalMaterial() const;

    virtual UObject* Duplicate() override;

protected:
    virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

private:
    UMaterial* DecalMaterial;
};