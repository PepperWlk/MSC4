
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "MapGenerator.generated.h"

UENUM(BlueprintType)
enum class EDesertBiome : uint8
{
    Desert,
    Oasis,
    Rocky,
    Forest,
    City
};

UCLASS()
class KENSHILIKE_API AMapGenerator : public AActor
{
    GENERATED_BODY()

public:
    AMapGenerator();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Desert")
    void GenerateDesert();

protected:
    virtual void OnConstruction(
        const FTransform& Transform
    ) override;

    // --------------------------------------------------
    // Meshes générés
    // --------------------------------------------------

    UPROPERTY(VisibleAnywhere, Category = "Desert")
    TObjectPtr<UProceduralMeshComponent> DesertMesh;

    UPROPERTY(VisibleAnywhere, Category = "Desert")
    TObjectPtr<UProceduralMeshComponent> WaterMesh;

    UPROPERTY(VisibleAnywhere, Category = "Desert")
    TObjectPtr<UProceduralMeshComponent> RoadMesh;

    UPROPERTY(VisibleAnywhere, Category = "Desert")
    TObjectPtr<UProceduralMeshComponent> CityMesh;

    // --------------------------------------------------
    // Instances optimisées
    // --------------------------------------------------

    UPROPERTY(VisibleAnywhere, Category = "Desert|Instances")
    TObjectPtr<UInstancedStaticMeshComponent> TreeInstances;

    UPROPERTY(VisibleAnywhere, Category = "Desert|Instances")
    TObjectPtr<UInstancedStaticMeshComponent> RockInstances;

    // --------------------------------------------------
    // Terrain : paramètres conservés
    // --------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Desert|Generation")
    int32 Seed = 12345;

    UPROPERTY(EditAnywhere, Category = "Desert|Generation",
        meta = (ClampMin = "2", ClampMax = "500"))
    int32 GridSize = 100;

    UPROPERTY(EditAnywhere, Category = "Desert|Generation",
        meta = (ClampMin = "10.0"))
    float CellSize = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Desert|Terrain",
        meta = (ClampMin = "0.0"))
    float HeightScale = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Desert|Terrain",
        meta = (ClampMin = "0.00001"))
    float NoiseScale = 0.015f;

    UPROPERTY(EditAnywhere, Category = "Desert|Terrain",
        meta = (ClampMin = "0.00001"))
    float DetailScale = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Desert|Terrain",
        meta = (ClampMin = "0.0"))
    float DetailStrength = 0.2f;

    // --------------------------------------------------
    // Répartition des biomes
    // --------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Desert|Biomes",
        meta = (ClampMin = "2", ClampMax = "10"))
    int32 BiomeRegionCount = 4;

    UPROPERTY(EditAnywhere, Category = "Desert|Biomes",
        meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float BiomeVariation = 0.18f;

    UPROPERTY(EditAnywhere, Category = "Desert|Biomes",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float OasisDensity = 0.35f;

    UPROPERTY(EditAnywhere, Category = "Desert|Biomes",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ForestDensity = 0.65f;

    UPROPERTY(EditAnywhere, Category = "Desert|Biomes",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RockDensity = 0.45f;

    // --------------------------------------------------
    // Ville
    // --------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Desert|City")
    bool bGenerateCity = true;

    UPROPERTY(EditAnywhere, Category = "Desert|City",
        meta = (ClampMin = "0.03", ClampMax = "0.30"))
    float CitySizeRatio = 0.12f;

    UPROPERTY(EditAnywhere, Category = "Desert|City",
        meta = (ClampMin = "300.0"))
    float StreetSpacing = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Desert|City",
        meta = (ClampMin = "100.0"))
    float StreetWidth = 220.0f;

    UPROPERTY(EditAnywhere, Category = "Desert|City",
        meta = (ClampMin = "100.0"))
    float MinBuildingHeight = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Desert|City",
        meta = (ClampMin = "100.0"))
    float MaxBuildingHeight = 800.0f;

    // --------------------------------------------------
    // Végétation et rochers
    // --------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Desert|Assets")
    TObjectPtr<UStaticMesh> TreeMesh;

    UPROPERTY(EditAnywhere, Category = "Desert|Assets")
    TObjectPtr<UStaticMesh> RockMesh;

    UPROPERTY(EditAnywhere, Category = "Desert|Vegetation",
        meta = (ClampMin = "1", ClampMax = "20"))
    int32 InstanceSampleStep = 4;

    // --------------------------------------------------
    // Matériaux
    // --------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Desert|Appearance")
    TObjectPtr<UMaterialInterface> DesertMaterial;

    UPROPERTY(EditAnywhere, Category = "Desert|Appearance")
    TObjectPtr<UMaterialInterface> WaterMaterial;

    UPROPERTY(EditAnywhere, Category = "Desert|Appearance")
    TObjectPtr<UMaterialInterface> RoadMaterial;

    UPROPERTY(EditAnywhere, Category = "Desert|Appearance")
    TObjectPtr<UMaterialInterface> BuildingMaterial;
};
