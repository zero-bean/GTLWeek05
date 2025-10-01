#include "pch.h"
#include "Render/UI/Widget/Public/SpriteSelectionWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Component/Public/BillBoardComponent.h"

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
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (CurrentLevel)
	{
		AActor* NewSelectedActor = CurrentLevel->GetSelectedActor();

		// Update Current Selected Actor
		if (SelectedActor != NewSelectedActor)
		{
			SelectedActor = NewSelectedActor;
		}

		// Get Current Selected Actor Information
		if (SelectedActor)
			UpdateSpriteFromActor();
	}
}

void USpriteSelectionWidget::RenderWidget()
{
	// Memory Information
	// ImGui::Text("레벨 메모리 정보");
	// ImGui::Text("Level Object Count: %u", LevelObjectCount);
	// ImGui::Text("Level Memory: %.3f KB", static_cast<float>(LevelMemoryByte) / KILO);
	// ImGui::Separator();

	if (!SelectedActor)
		return;
	
	ImGui::Separator();
	ImGui::Text("Select Sprite");

	ImGui::Spacing();
		
	static int current_item = 0; // 현재 선택된 인덱스

	// 예제 문자열 목록
	TArray<const char*> items;
	const TMap<FName, ID3D11ShaderResourceView*>& TextureCache = \
		UAssetManager::GetInstance().GetTextureCache();

	int i = 0;
	for (auto Itr = TextureCache.begin(); Itr != TextureCache.end(); Itr++, i++)
	{
		if (Itr->first == SelectedSpriteName)
			current_item = i;

		items.push_back(Itr->first.ToString().c_str());
	}

	if (ImGui::BeginCombo("Sprite", items[current_item])) // Label과 현재 값 표시
	{
		for (int n = 0; n < items.size(); n++)
		{
			bool is_selected = (current_item == n);
			if (ImGui::Selectable(items[n], is_selected))
			{
				current_item = n;
				SelectedSpriteName = items[current_item];
			}

			if (is_selected)
				ImGui::SetItemDefaultFocus(); // 기본 포커스
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();
}

/**
 * @brief Render에서 체크된 내용으로 인해 이후 변경되어야 할 내용이 있다면 Change 처리
 */
void USpriteSelectionWidget::PostProcess()
{
	SetSpriteOfActor();
}

void USpriteSelectionWidget::UpdateSpriteFromActor()
{
	for (const TObjectPtr<UActorComponent>& Component : SelectedActor->GetOwnedComponents())
	{
		TObjectPtr<UBillBoardComponent> UUIDTextComponent = Cast<UBillBoardComponent>(Component);
		if (UUIDTextComponent)
			SelectedSpriteName = UUIDTextComponent->GetSprite().first;
	}
}

void USpriteSelectionWidget::SetSpriteOfActor()
{
	const TMap<FName, ID3D11ShaderResourceView*>& TextureCache = \
		UAssetManager::GetInstance().GetTextureCache();

	for (const TObjectPtr<UActorComponent>& Component : SelectedActor->GetOwnedComponents())
	{
		TObjectPtr<UBillBoardComponent> UUIDTextComponent = Cast<UBillBoardComponent>(Component);
		if (UUIDTextComponent)
		{
			UUIDTextComponent->SetSprite(*TextureCache.find(SelectedSpriteName));
			break;
		}
	}
}
