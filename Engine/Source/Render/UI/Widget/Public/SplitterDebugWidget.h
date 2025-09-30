#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class SSplitter;

class USplitterDebugWidget : public UWidget
{
    GENERATED_BODY()
    DECLARE_CLASS(USplitterDebugWidget, UWidget)
    
public:
	USplitterDebugWidget();
	virtual ~USplitterDebugWidget() override = default;

	void Initialize() override {}
	void Update() override {}
	void RenderWidget() override;

private:
	const SSplitter* RootSplitter = nullptr;
	const SSplitter* LeftSplitter = nullptr;
	const SSplitter* RightSplitter = nullptr;
};
