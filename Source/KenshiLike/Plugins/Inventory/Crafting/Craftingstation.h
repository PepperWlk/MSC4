#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "CraftingStation.generated.h"

class UDataTable;
class UCraftingWidget;

UCLASS()
class ACraftingStation : public AActor
{
	GENERATED_BODY()

public:
	// À appeler depuis TON système d'interaction
	UFUNCTION(BlueprintCallable)
	void InteractBehavior(APawn* Interactor);

protected:
	UPROPERTY(EditAnywhere, Category = "Crafting")
	TObjectPtr<UDataTable> RecipeTable; // row type = FCraftingRecipe

	UPROPERTY(EditAnywhere, Category = "Crafting")
	FGameplayTag StationTag;

	UPROPERTY(EditAnywhere, Category = "Crafting")
	TSubclassOf<UCraftingWidget> WidgetClass;

private:
	UPROPERTY() TObjectPtr<UCraftingWidget> ActiveWidget;
};