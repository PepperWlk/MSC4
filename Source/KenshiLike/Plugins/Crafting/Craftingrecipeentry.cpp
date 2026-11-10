#include "CraftingRecipeEntry.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "ItemDefinitionBase.h"

void UCraftingRecipeEntry::NativeConstruct()
{
	Super::NativeConstruct();
	EntryButton->OnClicked.AddUniqueDynamic(this, &UCraftingRecipeEntry::HandleClicked);
}

void UCraftingRecipeEntry::Setup(FName InRowName, const FCraftingRecipe& Recipe)
{
	RowName = InRowName;
	if (!Recipe.ResultItem) return;

	NameText->SetText(FText::Format(NSLOCTEXT("Crafting", "Result", "{0} x{1}"),
		Recipe.ResultItem->GetDisplayName(), Recipe.ResultQuantity));

	if (IconImage)
	{
		if (UTexture2D* Tex = Recipe.ResultItem->GetIcon())
			IconImage->SetBrushFromTexture(Tex);
	}

	FString Ingredients;
	for (const FCraftingIngredient& Ing : Recipe.Ingredients)
	{
		if (!Ing.Item) continue;
		Ingredients += FString::Printf(TEXT("%dx %s\n"), Ing.Quantity, *Ing.Item->GetDisplayName().ToString());
	}
	IngredientsText->SetText(FText::FromString(Ingredients.TrimEnd()));
}

void UCraftingRecipeEntry::HandleClicked()
{
	OnClicked.ExecuteIfBound(RowName);
}