#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "CraftingTypes.generated.h"

class UItemDefinitionBase; // classe de base de PDA_ItemDefinition

USTRUCT(BlueprintType)
struct FCraftingIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UItemDefinitionBase> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 Quantity = 1;
};

// Une row du DataTable = une recette
USTRUCT(BlueprintType)
struct FCraftingRecipe : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCraftingIngredient> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UItemDefinitionBase> ResultItem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 ResultQuantity = 1;

	// Optionnel : forge, cuisine... (vide = utilisable partout)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RequiredStation;
};