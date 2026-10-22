#include "EnemySpawner.h"
#include "Base/EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	ScheduleNextSpawn(FirstSpawnDelay);
}

void AEnemySpawner::ScheduleNextSpawn(float Delay)
{
	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AEnemySpawner::SpawnGroup, Delay, false);
}

void AEnemySpawner::SpawnGroup()
{
	if (!MeleeEnemyClass && !RangedEnemyClass) return;

	if (AliveEnemies >= MaxAliveEnemies)
	{
		ScheduleNextSpawn(RetryDelay);
		return;
	}

	FVector Center;
	if (!FindSpawnCenter(Center))
	{
		// Aucun point invisible pour l'instant : on réessaie bientôt
		ScheduleNextSpawn(RetryDelay);
		return;
	}

	// Un point valide a été trouvé : on programme la prochaine vague normalement
	ScheduleNextSpawn(FMath::FRandRange(MinInterval, MaxInterval));

	const int32 GroupSize = FMath::Min(FMath::RandRange(MinGroupSize, MaxGroupSize),
	                                   MaxAliveEnemies - AliveEnemies);

	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());

	for (int32 i = 0; i < GroupSize; ++i)
	{
		TSubclassOf<AEnemyBase> ClassToSpawn =
			(FMath::FRand() < RangedRatio) ? RangedEnemyClass : MeleeEnemyClass;
		if (!ClassToSpawn) ClassToSpawn = RangedEnemyClass ? RangedEnemyClass : MeleeEnemyClass;

		// Position de l'ennemi autour du centre : elle doit AUSSI être invisible
		bool bFound = false;
		FVector SpawnPos = Center;
		for (int32 Try = 0; Try < 6 && !bFound; ++Try)
		{
			FNavLocation Loc;
			if (Nav && Nav->GetRandomReachablePointInRadius(Center, GroupRadius, Loc))
			{
				SpawnPos = Loc.Location;
			}
			bFound = !bSpawnInvisible || IsLocationHiddenFromPlayer(SpawnPos);
		}
		if (!bFound) continue; // on saute cet ennemi plutôt que de le faire apparaître à l'écran

		SpawnPos.Z += 100.f; // marge pour la capsule
		const FRotator Rot(0.f, FMath::FRandRange(0.f, 360.f), 0.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

		if (AEnemyBase* Enemy = GetWorld()->SpawnActor<AEnemyBase>(ClassToSpawn, SpawnPos, Rot, Params))
		{
			++AliveEnemies;
			Enemy->OnDestroyed.AddDynamic(this, &AEnemySpawner::OnEnemyDestroyed);
		}
	}
}

bool AEnemySpawner::FindSpawnCenter(FVector& OutCenter) const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player) return false;

	// "Le chemin du joueur" = sa direction de déplacement, sinon là où il regarde
	FVector Dir = Player->GetVelocity().GetSafeNormal2D();
	if (Dir.IsNearlyZero())
	{
		Dir = Player->GetActorForwardVector().GetSafeNormal2D();
	}

	// 1) Priorité : le cône devant le joueur
	if (TryFindPoint(Player->GetActorLocation(), Dir, SpawnConeHalfAngle, OutCenter))
	{
		return true;
	}

	// 2) Repli : tout autour du joueur (dans son dos, par exemple)
	if (bSpawnInvisible && bAllowSpawnAroundPlayer)
	{
		return TryFindPoint(Player->GetActorLocation(), Dir, 180.f, OutCenter);
	}

	return false;
}

bool AEnemySpawner::TryFindPoint(const FVector& PlayerLoc, const FVector& Dir, float HalfAngle, FVector& OutPoint) const
{
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) return false;

	for (int32 Try = 0; Try < MaxPositionTries; ++Try)
	{
		const float Angle = FMath::FRandRange(-HalfAngle, HalfAngle);
		const float Dist = FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);
		const FVector Candidate = PlayerLoc + Dir.RotateAngleAxis(Angle, FVector::UpVector) * Dist;

		FNavLocation NavLoc;
		if (!Nav->ProjectPointToNavigation(Candidate, NavLoc, FVector(500.f, 500.f, 500.f)))
		{
			continue;
		}

		if (bSpawnInvisible && !IsLocationHiddenFromPlayer(NavLoc.Location))
		{
			continue;
		}

		OutPoint = NavLoc.Location;
		return true;
	}
	return false;
}

bool AEnemySpawner::IsLocationHiddenFromPlayer(const FVector& Location) const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->PlayerCameraManager) return true;

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	// --- Test 1 : hors du champ de vision (cône FOV + marge) ---
	const FVector ToHead = (Location + FVector(0.f, 0.f, 170.f)) - CamLoc;
	const float CosAngle = FVector::DotProduct(CamRot.Vector(), ToHead.GetSafeNormal());
	const float HalfFovRad = FMath::DegreesToRadians(PC->PlayerCameraManager->GetFOVAngle() * 0.5f + ViewMarginDegrees);
	if (CosAngle < FMath::Cos(HalfFovRad))
	{
		return true; // derrière / sur le côté : le joueur ne le voit pas
	}

	// --- Test 2 : dans le champ de vision mais masqué par un obstacle ---
	// On trace vers les pieds ET la tête : caché seulement si les deux sont bloqués.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpawnVisibility), false);
	Params.AddIgnoredActor(PC->GetPawn());

	FHitResult Hit;
	const bool bFeetBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit, CamLoc, Location + FVector(0.f, 0.f, 30.f), ECC_Visibility, Params);
	const bool bHeadBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit, CamLoc, Location + FVector(0.f, 0.f, 170.f), ECC_Visibility, Params);

	return bFeetBlocked && bHeadBlocked;
}

void AEnemySpawner::OnEnemyDestroyed(AActor* DestroyedActor)
{
	AliveEnemies = FMath::Max(0, AliveEnemies - 1);
}