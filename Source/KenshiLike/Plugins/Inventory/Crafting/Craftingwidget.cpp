#include "CraftingWidget.h"
#include "CraftingComponent.h"
#include "CraftingRecipeEntry.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "ItemDefinitionBase.h"

void UCraftingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true); // nécessaire pour recevoir Échap
	CraftButton->OnClicked.AddUniqueDynamic(this, &UCraftingWidget::OnCraftClicked);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UCraftingWidget::OnCloseClicked);
}

void UCraftingWidget::Open(UDataTable* InTable, FGameplayTag InStation, UCraftingComponent* InCrafting)
{
	Table = InTable;
	Station = InStation;
	Crafting = InCrafting;

	AddToViewport();
	Refresh();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

void UCraftingWidget::Close()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
	RemoveFromParent();
}

FReply UCraftingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCraftingWidget::Refresh()
{
	RecipeList->ClearChildren();
	if (!Table || !Crafting || !EntryClass) return;

	bool bSelectedStillValid = false;
	for (const FName& Row : Crafting->GetCraftableRecipes(Table, Station))
	{
		const FCraftingRecipe* Recipe = Table->FindRow<FCraftingRecipe>(Row, TEXT("Refresh"));
		if (!Recipe) continue;

		UCraftingRecipeEntry* Entry = CreateWidget<UCraftingRecipeEntry>(this, EntryClass);
		Entry->Setup(Row, *Recipe);
		Entry->OnClicked.BindUObject(this, &UCraftingWidget::SelectRecipe);
		RecipeList->AddChild(Entry);

		if (Row == SelectedRecipe) bSelectedStillValid = true;
	}

	if (!bSelectedStillValid) ClearSelection();
}

void UCraftingWidget::SelectRecipe(FName RowName)
{
	const FCraftingRecipe* Recipe = Table ? Table->FindRow<FCraftingRecipe>(RowName, TEXT("Select")) : nullptr;
	if (!Recipe || !Recipe->ResultItem) return;

	SelectedRecipe = RowName;
	const UItemDefinitionBase* Item = Recipe->ResultItem;

	ResultNameText->SetText(Item->GetDisplayName());
	ResultStatsText->SetText(BuildStatsText(Item));
	if (ResultIcon)
	{
		if (UTexture2D* Tex = Item->GetIcon())
			ResultIcon->SetBrushFromTexture(Tex);
	}

	CraftButton->SetIsEnabled(true);
}

void UCraftingWidget::ClearSelection()
{
	SelectedRecipe = NAME_None;
	ResultNameText->SetText(FText::GetEmpty());
	ResultStatsText->SetText(FText::GetEmpty());
	CraftButton->SetIsEnabled(false);
}

FText UCraftingWidget::BuildStatsText(const UItemDefinitionBase* Item) const
{
	return Item->GetStatsText(); // défini dans PDA_ItemDefinition (BP)
}

void UCraftingWidget::OnCraftClicked()
{
	if (Crafting && Crafting->Craft(Table, SelectedRecipe, Station))
		Refresh(); // la recette disparaît si elle n'est plus faisable
}

void UCraftingWidget::OnCloseClicked()
{
	Close();
}