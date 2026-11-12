#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemDefinitionBase;

// Remplace le struct BP ST_InventoryItem
USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UItemDefinitionBase> ItemDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "0"))
	int32 Quantity = 0;

	bool IsEmpty() const { return ItemDefinition == nullptr || Quantity <= 0; }
	void Clear() { ItemDefinition = nullptr; Quantity = 0; }
};

// Broadcast à chaque modification : à binder dans l'UI pour se rafraîchir
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

// Remplace BP_InventoryComponent. Inventaire à slots fixes avec stacks.
UCLASS(ClassGroup = (Inventory), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// --- Réglages (reprends les valeurs de ton ancien BP) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxSlots = 20;

	// Utilisé si l'item ne définit pas son propre stack (GetMaxStack() <= 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxStack = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bAutoInitialize = true;

	// SwapItems fusionne les stacks si les deux slots contiennent le même item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bMergeStacksOnSwap = true;

	// Lecture seule en BP : modifie-le via les fonctions ci-dessous
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Inventory")
	TArray<FInventoryItem> Inventory;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	// --- API (mêmes noms que l'ancien BP) ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventory();

	// Ajoute autant que possible (stacks existants puis slots vides).
	// bSuccess = tout a été ajouté. RemainingQuantity = ce qui n'a pas tenu.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(UItemDefinitionBase* Item, int32 Quantity, bool& bSuccess, int32& RemainingQuantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CanAddItemInventory(UItemDefinitionBase* Item, int32 Quantity, bool& CanAdd) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapItems(int32 IndexA, int32 IndexB);

	// --- Nouveau (utilisé par le crafting) ---
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(const UItemDefinitionBase* Item) const;

	// Tout ou rien : retourne false sans rien retirer si la quantité n'est pas disponible
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemDefinitionBase* Item, int32 Quantity);

	// --- C++ uniquement ---
	bool CanAddItem(const UItemDefinitionBase* Item, int32 Quantity) const;
	const TArray<FInventoryItem>& GetSlots() const { return Inventory; }
	void RestoreSlots(const TArray<FInventoryItem>& Snapshot);

protected:
	virtual void BeginPlay() override;

private:
	int32 GetStackLimit(const UItemDefinitionBase* Item) const;
	int32 GetFreeSpaceFor(const UItemDefinitionBase* Item) const;
};