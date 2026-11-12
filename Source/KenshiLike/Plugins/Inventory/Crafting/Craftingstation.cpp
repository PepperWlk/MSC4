#include "CraftingStation.h"
#include "CraftingWidget.h"
#include "CraftingComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void ACraftingStation::InteractBehavior(APawn* Interactor)
{
	if (!Interactor || !WidgetClass || !RecipeTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACraftingStation::Interact - WidgetClass or RecipeTable missing"));
		return;
	}
	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	UCraftingComponent* Crafting = Interactor->FindComponentByClass<UCraftingComponent>();
	if (!PC || !PC->IsLocalController() || !Crafting) return;

	if (ActiveWidget && ActiveWidget->IsInViewport()) return; // déjà ouvert

	ActiveWidget = CreateWidget<UCraftingWidget>(PC, WidgetClass);
	ActiveWidget->Open(RecipeTable, StationTag, Crafting);
}