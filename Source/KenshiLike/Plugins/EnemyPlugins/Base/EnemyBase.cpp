#include "EnemyBase.h"
#include "EnemyAIController.h"
#include "EnemyProjectile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed =
		(EnemyType == EEnemyType::Melee) ? MeleeRunSpeed : RangedWalkSpeed;
}

bool AEnemyBase::CanAttack() const
{
	return !bIsDead && (GetWorld()->GetTimeSeconds() - LastAttackTime) >= AttackCooldown;
}

void AEnemyBase::PerformAttack(AActor* Target)
{
	if (!Target || !CanAttack()) return;
	LastAttackTime = GetWorld()->GetTimeSeconds();

	if (EnemyType == EEnemyType::Melee)
	{
		// TODO : jouer un montage d'attaque ici (PlayAnimMontage)
		UGameplayStatics::ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass());
	}
	else if (ProjectileClass)
	{
		const FVector Start = GetActorLocation() + GetActorForwardVector() * 80.f + FVector(0, 0, 40.f);
		const FVector TargetPoint = Target->GetActorLocation();
		const FRotator Aim = (TargetPoint - Start).Rotation();

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AEnemyProjectile* P = GetWorld()->SpawnActor<AEnemyProjectile>(ProjectileClass, Start, Aim, Params))
		{
			P->Damage = AttackDamage;
			// Évite que le projectile touche son lanceur
			if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(P->GetRootComponent()))
			{
				Root->IgnoreActorWhenMoving(this, true);
			}
		}
	}
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bIsDead) return 0.f;

	Health -= Applied;
	if (Health <= 0.f)
	{
		Die();
	}
	return Applied;
}

void AEnemyBase::Die()
{
	bIsDead = true;

	if (AController* C = GetController())
	{
		C->UnPossess();
		C->Destroy();
	}

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);

	SetLifeSpan(4.f); // Détruit => le spawner décrémente son compteur
}