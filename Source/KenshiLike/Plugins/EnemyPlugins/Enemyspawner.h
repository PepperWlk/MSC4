#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class AEnemyBase;

/**
 * Place UN seul de ces acteurs dans le niveau (n'importe où).
 * Il fait apparaître des groupes d'ennemis devant le joueur, à des intervalles aléatoires,
 * en dehors de son champ de vision ou derrière un obstacle.
 */
UCLASS()
class KENSHILIKE_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

	UPROPERTY(EditAnywhere, Category = "Spawn|Classes")
	TSubclassOf<AEnemyBase> MeleeEnemyClass;

	UPROPERTY(EditAnywhere, Category = "Spawn|Classes")
	TSubclassOf<AEnemyBase> RangedEnemyClass;

	/** Délai aléatoire entre deux apparitions de groupe (secondes). */
	UPROPERTY(EditAnywhere, Category = "Spawn|Timing")
	float MinInterval = 8.f;

	UPROPERTY(EditAnywhere, Category = "Spawn|Timing")
	float MaxInterval = 20.f;

	UPROPERTY(EditAnywhere, Category = "Spawn|Timing")
	float FirstSpawnDelay = 5.f;

	/** Si aucun point invisible n'est trouvé, on réessaie après ce délai (secondes). */
	UPROPERTY(EditAnywhere, Category = "Spawn|Timing")
	float RetryDelay = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Spawn|Group", meta = (ClampMin = "1"))
	int32 MinGroupSize = 2;

	UPROPERTY(EditAnywhere, Category = "Spawn|Group", meta = (ClampMin = "1"))
	int32 MaxGroupSize = 5;

	/** Proportion d'ennemis à distance dans un groupe (0 = que de la mêlée, 1 = que du tir). */
	UPROPERTY(EditAnywhere, Category = "Spawn|Group", meta = (ClampMin = "0", ClampMax = "1"))
	float RangedRatio = 0.4f;

	/** Rayon d'étalement du groupe autour du point d'apparition. */
	UPROPERTY(EditAnywhere, Category = "Spawn|Group")
	float GroupRadius = 300.f;

	/** Distance minimale / maximale par rapport au joueur. */
	UPROPERTY(EditAnywhere, Category = "Spawn|Position")
	float MinSpawnDistance = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Spawn|Position")
	float MaxSpawnDistance = 2500.f;

	/** Demi-angle du cône devant le joueur (degrés), selon sa direction de déplacement. */
	UPROPERTY(EditAnywhere, Category = "Spawn|Position")
	float SpawnConeHalfAngle = 35.f;

	// ---------- Spawn invisible ----------

	/** Si vrai, les ennemis n'apparaissent jamais dans le champ de vision du joueur. */
	UPROPERTY(EditAnywhere, Category = "Spawn|Visibility")
	bool bSpawnInvisible = true;

	/** Marge ajoutée au demi-FOV de la caméra (évite l'apparition juste au bord de l'écran). */
	UPROPERTY(EditAnywhere, Category = "Spawn|Visibility")
	float ViewMarginDegrees = 15.f;

	/**
	 * Si aucun point caché n'existe dans le cône devant le joueur
	 * (terrain dégagé), on cherche aussi tout autour de lui, par exemple dans son dos.
	 */
	UPROPERTY(EditAnywhere, Category = "Spawn|Visibility")
	bool bAllowSpawnAroundPlayer = true;

	/** Nombre d'essais pour trouver un point valide. */
	UPROPERTY(EditAnywhere, Category = "Spawn|Visibility", meta = (ClampMin = "1"))
	int32 MaxPositionTries = 20;

	/** Nombre max d'ennemis vivants en même temps. */
	UPROPERTY(EditAnywhere, Category = "Spawn|Limits")
	int32 MaxAliveEnemies = 15;

protected:
	virtual void BeginPlay() override;

	void ScheduleNextSpawn(float Delay);
	void SpawnGroup();

	bool FindSpawnCenter(FVector& OutCenter) const;
	bool TryFindPoint(const FVector& PlayerLoc, const FVector& Dir, float HalfAngle, FVector& OutPoint) const;

	/** Vrai si le point est hors du champ de vision du joueur OU caché derrière un obstacle. */
	bool IsLocationHiddenFromPlayer(const FVector& Location) const;

	UFUNCTION()
	void OnEnemyDestroyed(AActor* DestroyedActor);

	FTimerHandle SpawnTimer;
	int32 AliveEnemies = 0;
};