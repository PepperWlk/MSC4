#include "EnemyAIController.h"
#include "EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f; // 10 mises à jour/s suffisent pour l'IA
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StrafeSign = FMath::RandBool() ? 1.f : -1.f;
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AEnemyBase* Enemy = Cast<AEnemyBase>(GetPawn());
	AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Enemy || Enemy->IsDead() || !Player) return;

	const float Dist = FVector::Dist(Enemy->GetActorLocation(), Player->GetActorLocation());

	if (Enemy->EnemyType == EEnemyType::Melee)
		TickMelee(Enemy, Player, Dist);
	else
		TickRanged(Enemy, Player, Dist);
}

// ---------- Mêlée : fonce sur le joueur et frappe au contact ----------
void AEnemyAIController::TickMelee(AEnemyBase* Enemy, AActor* Player, float Dist)
{
	if (Dist > Enemy->AttackRange)
	{
		MoveToActor(Player, Enemy->AttackRange * 0.7f, true, true, true);
	}
	else
	{
		StopMovement();
		SetFocus(Player);
		Enemy->PerformAttack(Player);
	}
}

// ---------- Distance : s'arrête à bonne distance et tire ----------
void AEnemyAIController::TickRanged(AEnemyBase* Enemy, AActor* Player, float Dist)
{
	const bool bTooFar = Dist > Enemy->PreferredDistance;
	const bool bTooClose = Dist < Enemy->PreferredDistance * 0.5f;

	if (bTooFar || !HasLineOfSight(Player))
	{
		// Se rapproche jusqu'à avoir la distance voulue ET une ligne de tir
		ClearFocus(EAIFocusPriority::Gameplay);
		MoveToActor(Player, Enemy->PreferredDistance * 0.8f, true, true, true);
		return;
	}

	SetFocus(Player);

	if (bTooClose)
	{
		// Recule un peu pour garder ses distances
		const FVector Away = (Enemy->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
		MoveToLocation(Enemy->GetActorLocation() + Away * 400.f, 50.f);
	}
	else
	{
		StopMovement();
	}

	if (Dist <= Enemy->AttackRange)
	{
		Enemy->PerformAttack(Player);
	}
}

bool AEnemyAIController::HasLineOfSight(AActor* Target) const
{
	const APawn* Me = GetPawn();
	if (!Me || !Target) return false;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyLOS), false, Me);
	Params.AddIgnoredActor(Target);

	const FVector Start = Me->GetActorLocation() + FVector(0, 0, 60.f);
	const FVector End = Target->GetActorLocation();
	return !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}