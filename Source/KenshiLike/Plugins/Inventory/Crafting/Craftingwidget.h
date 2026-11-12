#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "CraftingWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UScrollBox;
class UDataTable;
class UCraftingComponent;
class UCraftingRecipeEntry;
class UItemDefinitionBase;

UCLASS(Abstract)
class UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Ajoute à l'écran, passe en mode UI, bloque mouvement/caméra
	void Open(UDataTable* InTable, FGameplayTag InStation, UCraftingComponent* InCrafting);
	void Close();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// Hook : construis ici le texte de stats à partir de TON PDA
	virtual FText BuildStatsText(const UItemDefinitionBase* Item) const;

	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TSubclassOf<UCraftingRecipeEntry> EntryClass;

	// --- Widgets à créer dans le WBP avec EXACTEMENT ces noms ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UScrollBox> RecipeList;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CraftButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CloseButton;     // la croix
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ResultNameText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ResultStatsText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ResultIcon;

private:
	void Refresh();
	void SelectRecipe(FName RowName);
	void ClearSelection();

	UFUNCTION() void OnCraftClicked();
	UFUNCTION() void OnCloseClicked();

	UPROPERTY() TObjectPtr<UDataTable> Table;
	UPROPERTY() TObjectPtr<UCraftingComponent> Crafting;
	FGameplayTag Station;
	FName SelectedRecipe = NAME_None;
};