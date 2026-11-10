#include "CraftingStation.h"
#include "CraftingWidget.h"
#include "CraftingComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void ACraftingStation::Interact(APawn* Interactor)
{
	if (!Interactor || !WidgetClass || !RecipeTable) return;

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	UCraftingComponent* Crafting = Interactor->FindComponentByClass<UCraftingComponent>();
	if (!PC || !PC->IsLocalController() || !Crafting) return;

	if (ActiveWidget && ActiveWidget->IsInViewport()) return; // déjà ouvert

	ActiveWidget = CreateWidget<UCraftingWidget>(PC, WidgetClass);
	ActiveWidget->Open(RecipeTable, StationTag, Crafting);
}