#include "EnemyProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AEnemyProjectile::AEnemyProjectile()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(15.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnHit);
	RootComponent = Collision;

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 1800.f;
	Movement->MaxSpeed = 1800.f;
	Movement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 5.f;
}

void AEnemyProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
							 FVector NormalImpulse, const FHitResult& Hit)
{
	// On ignore l'ennemi qui a tiré (et ses collègues)
	if (OtherActor && OtherActor != GetOwner() && OtherActor != this)
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage,
			GetInstigatorController(), this, UDamageType::StaticClass());
	}
	Destroy();
}