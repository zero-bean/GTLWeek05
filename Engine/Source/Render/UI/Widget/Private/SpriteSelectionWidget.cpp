#include "pch.h"
#include "Render/UI/Widget/Public/SpriteSelectionWidget.h"

#include "Level/Public/Level.h"
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
	// �� ������ Level�� ���õ� Actor�� Ȯ���ؼ� ���� �ݿ�
	ULevel* CurrentLevel = GWorld->GetLevel();

	if (CurrentLevel)
	{
		AActor* NewSelectedActor = GEditor->GetEditorModule()->GetSelectedActor();

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
	// ImGui::Text("���� �޸� ����");
	// ImGui::Text("Level Object Count: %u", LevelObjectCount);
	// ImGui::Text("Level Memory: %.3f KB", static_cast<float>(LevelMemoryByte) / KILO);
	// ImGui::Separator();

	if (!SelectedActor)
		return;
	
	ImGui::Separator();
	ImGui::Text("Select Sprite");

	ImGui::Spacing();
		
	static int current_item = 0; // ���� ���õ� �ε���

	// ���� ���ڿ� ���
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

	if (ImGui::BeginCombo("Sprite", items[current_item])) // Label�� ���� �� ǥ��
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
				ImGui::SetItemDefaultFocus(); // �⺻ ��Ŀ��
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();
}

/**
 * @brief Render���� üũ�� �������� ���� ���� ����Ǿ�� �� ������ �ִٸ� Change ó��
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
	if (!SelectedActor)
		return;

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
