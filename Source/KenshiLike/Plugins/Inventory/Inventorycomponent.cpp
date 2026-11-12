#include "InventoryComponent.h"
#include "Crafting/ItemDefinitionBase.h"

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoInitialize && Inventory.IsEmpty())
		InitializeInventory();
}

void UInventoryComponent::InitializeInventory()
{
	Inventory.Reset();
	Inventory.SetNum(FMath::Max(1, MaxSlots));
	OnInventoryChanged.Broadcast();
}

int32 UInventoryComponent::GetStackLimit(const UItemDefinitionBase* Item) const
{
	const int32 FromItem = Item ? Item->GetMaxStack() : 0;
	return FromItem > 0 ? FromItem : FMath::Max(1, MaxStack);
}

int32 UInventoryComponent::GetFreeSpaceFor(const UItemDefinitionBase* Item) const
{
	const int32 Stack = GetStackLimit(Item);
	int32 Space = 0;
	for (const FInventoryItem& Slot : Inventory)
	{
		if (Slot.IsEmpty())
			Space += Stack;
		else if (Slot.ItemDefinition == Item && Slot.Quantity < Stack)
			Space += Stack - Slot.Quantity;
	}
	return Space;
}

bool UInventoryComponent::CanAddItem(const UItemDefinitionBase* Item, int32 Quantity) const
{
	return Item && Quantity > 0 && GetFreeSpaceFor(Item) >= Quantity;
}

void UInventoryComponent::CanAddItemInventory(UItemDefinitionBase* Item, int32 Quantity, bool& CanAdd) const
{
	CanAdd = CanAddItem(Item, Quantity);
}

void UInventoryComponent::AddItem(UItemDefinitionBase* Item, int32 Quantity, bool& bSuccess, int32& RemainingQuantity)
{
	RemainingQuantity = FMath::Max(0, Quantity);
	bSuccess = false;
	if (!Item || Quantity <= 0) return;

	const int32 Stack = GetStackLimit(Item);

	// 1) Compléter les stacks existants
	for (FInventoryItem& Slot : Inventory)
	{
		if (RemainingQuantity <= 0) break;
		if (!Slot.IsEmpty() && Slot.ItemDefinition == Item && Slot.Quantity < Stack)
		{
			const int32 ToAdd = FMath::Min(Stack - Slot.Quantity, RemainingQuantity);
			Slot.Quantity += ToAdd;
			RemainingQuantity -= ToAdd;
		}
	}

	// 2) Remplir les slots vides
	for (FInventoryItem& Slot : Inventory)
	{
		if (RemainingQuantity <= 0) break;
		if (Slot.IsEmpty())
		{
			const int32 ToAdd = FMath::Min(Stack, RemainingQuantity);
			Slot.ItemDefinition = Item;
			Slot.Quantity = ToAdd;
			RemainingQuantity -= ToAdd;
		}
	}

	bSuccess = (RemainingQuantity <= 0);
	if (RemainingQuantity < Quantity)
		OnInventoryChanged.Broadcast();
}

int32 UInventoryComponent::GetItemCount(const UItemDefinitionBase* Item) const
{
	if (!Item) return 0;
	int32 Total = 0;
	for (const FInventoryItem& Slot : Inventory)
	{
		if (!Slot.IsEmpty() && Slot.ItemDefinition == Item)
			Total += Slot.Quantity;
	}
	return Total;
}

bool UInventoryComponent::RemoveItem(UItemDefinitionBase* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0 || GetItemCount(Item) < Quantity) return false;

	int32 Remaining = Quantity;
	// On vide en partant de la fin pour garder les premiers slots stables
	for (int32 i = Inventory.Num() - 1; i >= 0 && Remaining > 0; --i)
	{
		FInventoryItem& Slot = Inventory[i];
		if (Slot.IsEmpty() || Slot.ItemDefinition != Item) continue;

		const int32 ToRemove = FMath::Min(Slot.Quantity, Remaining);
		Slot.Quantity -= ToRemove;
		Remaining -= ToRemove;
		if (Slot.Quantity <= 0) Slot.Clear();
	}

	OnInventoryChanged.Broadcast();
	return true;
}

void UInventoryComponent::SwapItems(int32 IndexA, int32 IndexB)
{
	if (IndexA == IndexB || !Inventory.IsValidIndex(IndexA) || !Inventory.IsValidIndex(IndexB)) return;

	FInventoryItem& A = Inventory[IndexA];
	FInventoryItem& B = Inventory[IndexB];

	if (bMergeStacksOnSwap && !A.IsEmpty() && !B.IsEmpty() && A.ItemDefinition == B.ItemDefinition)
	{
		const int32 Move = FMath::Min(GetStackLimit(B.ItemDefinition) - B.Quantity, A.Quantity);
		if (Move > 0)
		{
			B.Quantity += Move;
			A.Quantity -= Move;
			if (A.Quantity <= 0) A.Clear();
			OnInventoryChanged.Broadcast();
			return;
		}
	}

	Swap(A, B);
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::RestoreSlots(const TArray<FInventoryItem>& Snapshot)
{
	Inventory = Snapshot;
	OnInventoryChanged.Broadcast();
}