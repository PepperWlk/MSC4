#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ScatterCactus.generated.h"

UCLASS()
class KENSHILIKE_API AScatterCactus : public AActor
{
	GENERATED_BODY()
    
public:
	AScatterCactus();

	// Composant HISM pour les cactus
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scatter")
	UHierarchicalInstancedStaticMeshComponent* CactusHISM;

	// L'acteur cible (ex: votre mesh de désert) sélectionnable dans l'éditeur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	AActor* TargetActor;

	// Nombre de cactus à générer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	int32 NumberOfCactus = 100;

	// Hauteur du lancer de rayon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	float RaycastHeight = 2000.0f;

	// Fonction cliquable dans l'éditeur
	UFUNCTION(CallInEditor, Category = "Scatter")
	void ScatterMeshes();
};