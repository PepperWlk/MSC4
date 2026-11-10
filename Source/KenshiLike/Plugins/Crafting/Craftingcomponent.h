#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingTypes.h"
#include "CraftingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftCompleted, FName, RecipeRow);

// À mettre sur le Character / PlayerState (même Actor que l'inventaire)
UCLASS(ClassGroup = (Crafting), meta = (BlueprintSpawnableComponent))
class UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	bool CanCraft(const FCraftingRecipe& Recipe) const;

	// Noms des rows faisables avec l'inventaire actuel
	UFUNCTION(BlueprintCallable)
	TArray<FName> GetCraftableRecipes(const UDataTable* Table, FGameplayTag Station) const;

	UFUNCTION(BlueprintCallable)
	bool Craft(const UDataTable* Table, FName RowName, FGameplayTag Station);

	UPROPERTY(BlueprintAssignable)
	FOnCraftCompleted OnCraftCompleted;

private:
	UObject* GetInventory() const; // composant BP implémentant ICraftingInventory
	bool StationMatches(const FCraftingRecipe& Recipe, FGameplayTag Station) const;
};