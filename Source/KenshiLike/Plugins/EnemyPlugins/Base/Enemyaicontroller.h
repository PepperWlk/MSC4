#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class AEnemyBase;

UCLASS()
class KENSHILIKE_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	void TickMelee(AEnemyBase* Enemy, AActor* Player, float Dist);
	void TickRanged(AEnemyBase* Enemy, AActor* Player, float Dist);
	bool HasLineOfSight(AActor* Target) const;

	// Léger décalage aléatoire pour que le groupe ne se superpose pas
	float StrafeSign = 1.f;
};