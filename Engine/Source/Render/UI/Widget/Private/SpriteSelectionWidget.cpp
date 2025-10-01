#include "pch.h"
#include "Render/UI/Widget/Public/SpriteSelectionWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Component/Public/BillBoardComponent.h"

#include <climits>

IMPLEMENT_CLASS(USpriteSelectionWidget, UWidget)

USpriteSelectionWidget::USpriteSelectionWidget()
	: UWidget("Sprite Selection Widget")
{
}

USpriteSelectionWidget::~USpriteSelectionWidget() = default;

void USpriteSelectionWidget::Initialize()
{
	// Do Nothing Here
}

void USpriteSelectionWidget::Update()
{
	// 매 프레임 Level의 선택된 Actor를 확인해서 정보 반영
	ULevel* CurrentLevel = GWorld->GetLevel();

	if (CurrentLevel)
	{
		AActor* NewSelectedActor = CurrentLevel->GetSelectedActor();

		// Update Current Selected Actor
		if (SelectedActor != NewSelectedActor)
		{
			SelectedActor = NewSelectedActor;

			for (const TObjectPtr<UActorComponent>& Component : SelectedActor->GetOwnedComponents())
			{
				TObjectPtr<UBillBoardComponent> UUIDTextComponent = Cast<UBillBoardComponent>(Component);
				if (UUIDTextComponent)
					SelectedBillBoard = UUIDTextComponent.Get();
			}
		}
	}
}

void USpriteSelectionWidget::RenderWidget()
{
	if (!SelectedActor)
		return;
	
	ImGui::Separator();
	ImGui::Text("Select Sprite");

	ImGui::Spacing();
		
	static int current_item = 0; // 현재 선택된 인덱스

	// 예제 문자열 목록
	TArray<FString> items;
	const TMap<FName, ID3D11ShaderResourceView*>& TextureCache = \
		UAssetManager::GetInstance().GetTextureCache();

	int i = 0;
	for (auto Itr = TextureCache.begin(); Itr != TextureCache.end(); Itr++, i++)
	{
		if (Itr->first == SelectedBillBoard->GetSprite().first)
			current_item = i;

		items.push_back(Itr->first.ToString());
	}

	sort(items.begin(), items.end());
	
	if (ImGui::BeginCombo("Sprite", items[current_item].c_str())) // Label과 현재 값 표시
	{
		for (int n = 0; n < items.size(); n++)
		{
			bool is_selected = (current_item == n);
			if (ImGui::Selectable(items[n].c_str(), is_selected))
			{
				current_item = n;
				SetSpriteOfActor(items[current_item]);
			}

			if (is_selected)
				ImGui::SetItemDefaultFocus(); // 기본 포커스
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();

	WidgetNum = (WidgetNum + 1) % std::numeric_limits<uint32>::max();
}

void USpriteSelectionWidget::SetSpriteOfActor(FString NewSprite)
{
	if (!SelectedActor)
		return;
	if (!SelectedBillBoard)
		return;

	const TMap<FName, ID3D11ShaderResourceView*>& TextureCache = \
		UAssetManager::GetInstance().GetTextureCache();

	SelectedBillBoard->SetSprite(*TextureCache.find(NewSprite));
}
