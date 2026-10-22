#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

class AEnemyProjectile;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Melee   UMETA(DisplayName = "Mêlée (fonce sur le joueur)"),
	Ranged  UMETA(DisplayName = "Distance (tire de loin)")
};

UCLASS()
class KENSHILIKE_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

	// --- Réglages (modifiables dans le Blueprint enfant) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyType EnemyType = EEnemyType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MaxHealth = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float AttackCooldown = 1.2f;

	/** Mêlée : portée du coup. Distance : portée max du tir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float AttackRange = 150.f;

	/** Distance : l'ennemi s'arrête à cette distance du joueur pour tirer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ranged")
	float PreferredDistance = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ranged")
	TSubclassOf<AEnemyProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Enemy|Movement")
	float MeleeRunSpeed = 650.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Movement")
	float RangedWalkSpeed = 300.f;

	// --- API utilisée par l'IA ---
	bool CanAttack() const;
	void PerformAttack(AActor* Target);
	bool IsDead() const { return bIsDead; }

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
							 AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	void Die();

	float Health = 0.f;
	float LastAttackTime = -1000.f;
	bool bIsDead = false;
};