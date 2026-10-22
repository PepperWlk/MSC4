#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "MapGenerator.generated.h"

UENUM(BlueprintType)
enum class EMapQuadrant : uint8
{
    TopLeft     UMETA(DisplayName = "Nord-Ouest (NW)"),
    TopRight    UMETA(DisplayName = "Nord-Est (NE)"),
    BottomLeft  UMETA(DisplayName = "Sud-Ouest (SW)"),
    BottomRight UMETA(DisplayName = "Sud-Est (SE)")
};

UENUM(BlueprintType)
enum class EDesertBiome : uint8
{
    Desert,
    Oasis,
    Rocky
};

struct FDesertOasis
{
    float X = 0.0f;
    float Y = 0.0f;
    float Radius = 0.0f;
    float ShapeX = 0.0f;
    float ShapeY = 0.0f;
};

UCLASS()
class KENSHILIKE_API AMapGenerator : public AActor
{
    GENERATED_BODY()

public:
    AMapGenerator();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Map")
    void GenerateMap();

    UFUNCTION(BlueprintCallable, Category = "Map")
    float GetTerrainHeightAtWorld(float WorldX, float WorldY) const;

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, Category = "Map")
    TObjectPtr<UProceduralMeshComponent> DesertMesh;

    UPROPERTY(VisibleAnywhere, Category = "Map")
    TObjectPtr<UProceduralMeshComponent> WaterMesh;

    UPROPERTY(EditAnywhere, Category = "Map|Generation")
    bool bAutoGenerate = true;

    UPROPERTY(EditAnywhere, Category = "Map|Quadrant")
    EMapQuadrant Quadrant = EMapQuadrant::TopLeft;

    UPROPERTY(EditAnywhere, Category = "Map|Generation", meta = (ClampMin = "2", ClampMax = "150"))
    int32 GridSize = 100;

    UPROPERTY(EditAnywhere, Category = "Map|Generation", meta = (ClampMin = "10.0"))
    float CellSize = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Map|Generation")
    int32 MapSeed = 1337;

    // Terrain Désert
    UPROPERTY(EditAnywhere, Category = "Map|Terrain", meta = (ClampMin = "0.0"))
    float HeightScale = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain", meta = (ClampMin = "0.00001"))
    float NoiseScale = 0.015f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain", meta = (ClampMin = "0.00001"))
    float DetailScale = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain", meta = (ClampMin = "0.0"))
    float DetailStrength = 0.2f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain|Dunes", meta = (ClampMin = "0.0"))
    float DuneHeight = 90.0f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain|Dunes", meta = (ClampMin = "0.001"))
    float DuneFrequency = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain|Dunes", meta = (ClampMin = "0.0", ClampMax = "360.0"))
    float DuneWindAngle = 35.0f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain|Dunes", meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float DuneStretch = 0.35f;

    // Terrain Grand Canyon
    UPROPERTY(EditAnywhere, Category = "Map|Terrain|Canyon", meta = (ClampMin = "0.1"))
    float RockyHeightMultiplier = 1.8f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain|Canyon", meta = (ClampMin = "0.0"))
    float CanyonDepth = 1200.0f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain|Canyon", meta = (ClampMin = "0.0"))
    float TerraceStep = 140.0f;

    UPROPERTY(EditAnywhere, Category = "Map|Terrain", meta = (ClampMin = "10.0"))
    float TerrainUVWorldSize = 1000.0f;

    // Proportions : 3/4 Désert, 1/4 Canyon
    UPROPERTY(EditAnywhere, Category = "Map|Biomes", meta = (ClampMin = "0.5", ClampMax = "0.9"))
    float RockTransitionStart = 0.75f;

    UPROPERTY(EditAnywhere, Category = "Map|Biomes", meta = (ClampMin = "0.02", ClampMax = "0.2"))
    float RockTransitionWidth = 0.08f;

    // Oasis Profondes et Large (Largement augmentées)
    UPROPERTY(EditAnywhere, Category = "Map|Oasis", meta = (ClampMin = "1", ClampMax = "20"))
    int32 OasisCount = 5;

    UPROPERTY(EditAnywhere, Category = "Map|Oasis", meta = (ClampMin = "500.0"))
    float OasisRadius = 2500.0f; // Plus large

    UPROPERTY(EditAnywhere, Category = "Map|Oasis", meta = (ClampMin = "10.0", ClampMax = "500.0"))
    float OasisDepth = 200.0f; // Beaucoup plus profond

    UPROPERTY(EditAnywhere, Category = "Map|Oasis", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    float WaterRadiusRatio = 2.2f; // L'eau s'étale sur une très large portion de l'oasis

    // Matériaux
    UPROPERTY(EditAnywhere, Category = "Map|Appearance")
    TObjectPtr<UMaterialInterface> DesertMaterial;

    UPROPERTY(EditAnywhere, Category = "Map|Appearance")
    TObjectPtr<UMaterialInterface> WaterMaterial;

private:
    void ClearGenerated();
    void ApplyMaterials();
    void BuildTerrainMesh();
    void BuildWater();

    TArray<FDesertOasis> GenerateGlobalOases(float GlobalMapExtent) const;
    float OasisNormalizedDistance(float X, float Y, const TArray<FDesertOasis>& OasesList) const;

    void ComputeBiomeWeights(float X, float Y, float GlobalMapExtent, float* OutW) const;
    float ComputeHeight(float X, float Y, float GlobalMapExtent, const float* W, float OasisDist) const;

    float SampleHeightAtWorld(float WorldX, float WorldY, float GlobalMapExtent) const;
    FVector SampleNormalAtWorld(float WorldX, float WorldY, float GlobalMapExtent) const;

    TArray<FLinearColor> TerrainTint;
    TArray<uint8> BlockedGrid;
    TArray<EDesertBiome> BiomeGrid;
    TArray<float> HeightGrid;

    int32 GridVerts = 0;
};