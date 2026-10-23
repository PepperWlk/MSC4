#include "PlayerStatsComponent.h"

UPlayerStatsComponent::UPlayerStatsComponent()
{
	// Activer le Tick pour gérer la diminution au fil du temps
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialiser les stats au maximum au lancement
	CurrentThirst = MaxThirst;
	CurrentHunger = MaxHunger;
	CurrentStamina = MaxStamina;
}

void UPlayerStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 1. Gestion de la soif avec le temps
	if (CurrentThirst > 0.0f)
	{
		CurrentThirst = FMath::Clamp(CurrentThirst - (ThirstDecreaseRate * DeltaTime), 0.0f, MaxThirst);
        
		if (CurrentThirst <= 0.0f)
		{
			// TODO: Déclencher les dégâts de déshydratation ici
			UE_LOG(LogTemp, Warning, TEXT("Le joueur est déshydraté !"));
			isDehydrated = true;
		}
	}

	// 2. Gestion de la faim avec le temps
	if (CurrentHunger > 0.0f)
	{
		CurrentHunger = FMath::Clamp(CurrentHunger - (HungerDecreaseRate * DeltaTime), 0.0f, MaxHunger);
        
		if (CurrentHunger <= 0.0f)
		{
			// TODO: Déclencher les dégâts de faim ici
			UE_LOG(LogTemp, Warning, TEXT("Le joueur est affamé !"));
			isHungry = true;
		}
	}
}

void UPlayerStatsComponent::ModifyThirst(float Amount)
{
	CurrentThirst = FMath::Clamp(CurrentThirst + Amount, 0.0f, MaxThirst);
	UE_LOG(LogTemp, Log, TEXT("Soif modifiée de %.1f. Valeur actuelle : %.1f"), Amount, CurrentThirst);
}

void UPlayerStatsComponent::ModifyHunger(float Amount)
{
	CurrentHunger = FMath::Clamp(CurrentHunger + Amount, 0.0f, MaxHunger);
}

void UPlayerStatsComponent::ModifyStamina(float Amount)
{
	CurrentStamina = FMath::Clamp(CurrentStamina + Amount, 0.0f, MaxStamina);
}