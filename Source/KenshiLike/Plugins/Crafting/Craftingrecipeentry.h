#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingTypes.h"
#include "CraftingRecipeEntry.generated.h"

class UButton;
class UTextBlock;
class UImage;

DECLARE_DELEGATE_OneParam(FOnRecipeEntryClicked, FName /*RowName*/);

// Une ligne de la liste. Crée un WBP enfant de cette classe et nomme les widgets comme ci-dessous.
UCLASS(Abstract)
class UCraftingRecipeEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(FName InRowName, const FCraftingRecipe& Recipe);
	FOnRecipeEntryClicked OnClicked;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> EntryButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> IngredientsText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IconImage;

private:
	UFUNCTION() void HandleClicked();
	FName RowName;
};