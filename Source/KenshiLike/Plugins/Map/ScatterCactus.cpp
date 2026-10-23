#include "ScatterCactus.h"
#include "Engine/World.h"

AScatterCactus::AScatterCactus()
{
    PrimaryActorTick.bCanEverTick = false;

    CactusHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CactusHISM"));
    RootComponent = CactusHISM;
}

void AScatterCactus::ScatterMeshes()
{
    if (!CactusHISM || !CactusHISM->GetStaticMesh())
    {
        UE_LOG(LogTemp, Warning, TEXT("Veuillez assigner un StaticMesh au composant HISM des cactus."));
        return;
    }

    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Veuillez sélectionner un Target Actor (votre désert) dans les détails."));
        return;
    }

    // Nettoyer les anciennes instances avant de régénérer
    CactusHISM->ClearInstances();

    // Récupérer la boîte englobante globale de l'acteur cible (couvre tout le mesh)
    FBox Bounds = TargetActor->GetComponentsBoundingBox(true);
    if (!Bounds.IsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("L'acteur cible ne possède pas de bounding box valide."));
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    int32 SuccessfulSpawns = 0;
    int32 MaxAttempts = NumberOfCactus * 3; // Sécurité pour éviter les boucles infinies
    int32 Attempts = 0;

    // On utilise une boucle pour s'assurer d'atteindre le nombre exact de cactus demandés
    while (SuccessfulSpawns < NumberOfCactus && Attempts < MaxAttempts)
    {
        Attempts++;

        // Coordonnées aléatoires sur toute la largeur/longueur de la boîte du désert
        float RandomX = FMath::RandRange(Bounds.Min.X, Bounds.Max.X);
        float RandomY = FMath::RandRange(Bounds.Min.Y, Bounds.Max.Y);
        float RandomZ = Bounds.Max.Z + RaycastHeight;

        FVector StartLocation(RandomX, RandomY, RandomZ);
        FVector EndLocation(RandomX, RandomY, Bounds.Min.Z - RaycastHeight);

        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        QueryParams.AddIgnoredActor(TargetActor); // Optionnel selon le setup, mais évite les interférences

        // Lancer le rayon vers le bas
        bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, QueryParams);

        // On vérifie que le rayon a touché l'acteur cible (le désert)
        if (bHit && HitResult.GetActor() == TargetActor)
        {
            FVector SpawnLocation = HitResult.Location;

            // Rotation aléatoire sur l'axe Yaw
            FRotator SpawnRotation(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);

            // Échelle légèrement variée
            float RandomScale = FMath::RandRange(0.7f, 1.3f);
            FVector SpawnScale(RandomScale);

            FTransform InstanceTransform(SpawnRotation, SpawnLocation, SpawnScale);
            CactusHISM->AddInstance(InstanceTransform);
            SuccessfulSpawns++;
        }
    }
}