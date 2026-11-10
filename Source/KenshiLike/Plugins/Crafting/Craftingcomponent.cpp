#include "CraftingComponent.h"
#include "GameFramework/Actor.h"

UObject* UCraftingComponent::GetInventory() const
{
	if (!GetOwner()) return nullptr;
	// Trouve le composant (BP) de l'owner qui implémente l'interface
	return GetOwner()->FindComponentByInterface(UCraftingInventory::StaticClass());
}

bool UCraftingComponent::StationMatches(const FCraftingRecipe& Recipe, FGameplayTag Station) const
{
	return !Recipe.RequiredStation.IsValid() || Recipe.RequiredStation.MatchesTagExact(Station);
}

bool UCraftingComponent::CanCraft(const FCraftingRecipe& Recipe) const
{
	UObject* Inv = GetInventory();
	if (!Inv || !Recipe.ResultItem || Recipe.Ingredients.IsEmpty()) return false;

	// Fusionne les ingrédients dupliqués avant de comparer
	TMap<UItemDefinitionBase*, int32> Needed;
	for (const FCraftingIngredient& Ing : Recipe.Ingredients)
	{
		if (!Ing.Item) return false;
		Needed.FindOrAdd(Ing.Item) += Ing.Quantity;
	}
	for (const TPair<UItemDefinitionBase*, int32>& P : Needed)
	{
		if (ICraftingInventory::Execute_CraftingGetItemCount(Inv, P.Key) < P.Value) return false;
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
	UObject* Inv = GetInventory();
	if (!Recipe || !Inv) return false;

	// Toujours revérifier côté logique, même si l'UI a déjà filtré
	if (!StationMatches(*Recipe, Station) || !CanCraft(*Recipe)) return false;
	if (!ICraftingInventory::Execute_CraftingCanAddItem(Inv, Recipe->ResultItem, Recipe->ResultQuantity)) return false; // place dispo ?

	for (const FCraftingIngredient& Ing : Recipe->Ingredients)
		ICraftingInventory::Execute_CraftingRemoveItem(Inv, Ing.Item, Ing.Quantity);

	ICraftingInventory::Execute_CraftingAddItem(Inv, Recipe->ResultItem, Recipe->ResultQuantity);
	OnCraftCompleted.Broadcast(RowName);
	return true;
}