#include "CraftingComponent.h"
#include "../InventoryComponent.h"
#include "ItemDefinitionBase.h"
#include "GameFramework/Actor.h"

UInventoryComponent* UCraftingComponent::GetInventory() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
}

bool UCraftingComponent::StationMatches(const FCraftingRecipe& Recipe, FGameplayTag Station) const
{
	return !Recipe.RequiredStation.IsValid() || Recipe.RequiredStation.MatchesTagExact(Station);
}

bool UCraftingComponent::CanCraft(const FCraftingRecipe& Recipe) const
{
	const UInventoryComponent* Inv = GetInventory();
	if (!Inv || !Recipe.ResultItem || Recipe.Ingredients.IsEmpty()) return false;

	// Fusionne les ingrédients dupliqués avant de comparer
	TMap<const UItemDefinitionBase*, int32> Needed;
	for (const FCraftingIngredient& Ing : Recipe.Ingredients)
	{
		if (!Ing.Item) return false;
		Needed.FindOrAdd(Ing.Item) += Ing.Quantity;
	}
	for (const TPair<const UItemDefinitionBase*, int32>& P : Needed)
	{
		if (Inv->GetItemCount(P.Key) < P.Value) return false;
	}
	return true;
}

TArray<FName> UCraftingComponent::GetCraftableRecipes(const UDataTable* Table, FGameplayTag Station) const
{
	TArray<FName> Result;
	if (!Table) return Result;

	Table->ForeachRow<FCraftingRecipe>(TEXT("GetCraftableRecipes"),
		[&](const FName& Key, const FCraftingRecipe& Recipe)
		{
			if (StationMatches(Recipe, Station) && CanCraft(Recipe))
				Result.Add(Key);
		});
	return Result;
}

bool UCraftingComponent::Craft(const UDataTable* Table, FName RowName, FGameplayTag Station)
{
	// MULTI : if (!GetOwner()->HasAuthority()) return false; + passer par un RPC Server
	if (!Table) return false;
	const FCraftingRecipe* Recipe = Table->FindRow<FCraftingRecipe>(RowName, TEXT("Craft"));
	UInventoryComponent* Inv = GetInventory();
	if (!Recipe || !Inv) return false;

	// Toujours revérifier côté logique, même si l'UI a déjà filtré
	if (!StationMatches(*Recipe, Station) || !CanCraft(*Recipe)) return false;

	// On retire d'abord (ça peut libérer des slots), puis on vérifie la place.
	// Si le résultat ne tient pas : rollback, le joueur ne perd rien.
	const TArray<FInventoryItem> Snapshot = Inv->GetSlots();

	for (const FCraftingIngredient& Ing : Recipe->Ingredients)
		Inv->RemoveItem(Ing.Item, Ing.Quantity);

	if (!Inv->CanAddItem(Recipe->ResultItem, Recipe->ResultQuantity))
	{
		Inv->RestoreSlots(Snapshot);
		return false;
	}

	bool bSuccess = false;
	int32 Remaining = 0;
	Inv->AddItem(Recipe->ResultItem, Recipe->ResultQuantity, bSuccess, Remaining);

	OnCraftCompleted.Broadcast(RowName);
	return true;
}