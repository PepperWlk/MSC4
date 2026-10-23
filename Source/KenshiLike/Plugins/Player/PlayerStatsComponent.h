#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStatsComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class KENSHILIKE_API UPlayerStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UPlayerStatsComponent();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- SOIF ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Thirst")
    float MaxThirst = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Thirst")
    float CurrentThirst;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Thirst")
    float ThirstDecreaseRate = 0.3f; // Points perdus par seconde

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Thirst")
    bool isDehydrated = false;
    
    // --- FAIM ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Hunger")
    float MaxHunger = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Hunger")
    float CurrentHunger;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Hunger")
    float HungerDecreaseRate = 0.5f; // Points perdus par seconde
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Hunger")
    bool isHungry = false;

    // --- ENDURANCE (STAMINA) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina")
    float MaxStamina = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Stamina")
    float CurrentStamina;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina")
    float StaminaDrainRate = 20.0f; // Consommation en cas d'effort

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina")
    float StaminaRegenRate = 15.0f; // Régénération par seconde au repos

    // --- FONCTIONS DE MODIFICATION ---
    UFUNCTION(BlueprintCallable, Category = "Stats|Actions")
    void ModifyThirst(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Stats|Actions")
    void ModifyHunger(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Stats|Actions")
    void ModifyStamina(float Amount);
};