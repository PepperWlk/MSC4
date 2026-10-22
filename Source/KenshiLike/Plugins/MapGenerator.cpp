#include "MapGenerator.h"

#include "Engine/StaticMesh.h"

namespace
{
    constexpr int32 WaterSegments = 48;
    constexpr float MaxOasisInfluence = 4.0f; // Étendue de l'influence pour accepter de plus grands rayons

    inline float Smooth01(float T)
    {
        T = FMath::Clamp(T, 0.0f, 1.0f);
        return T * T * (3.0f - 2.0f * T);
    }

    inline FLinearColor LerpColor(const FLinearColor& A, const FLinearColor& B, float T)
    {
        return A + (B - A) * T;
    }

    inline FColor ToVertexColor(const FLinearColor& C)
    {
        return FLinearColor(C.R, C.G, C.B, 1.0f).ToFColor(false);
    }

    float OasisShapeFactor(const FDesertOasis& O, float Angle)
    {
        const float Nx = FMath::Cos(Angle) * 1.4f + O.ShapeX;
        const float Ny = FMath::Sin(Angle) * 1.4f + O.ShapeY;
        return 1.0f + 0.28f * FMath::PerlinNoise2D(FVector2D(Nx, Ny));
    }

    struct FMeshData
    {
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> Colors;
        TArray<FProcMeshTangent> Tangents;

        bool IsEmpty() const
        {
            return Vertices.IsEmpty() || Triangles.IsEmpty();
        }

        int32 AddVertex(const FVector& P, const FVector& N, const FVector2D& UV,
            const FColor& Color, const FVector& TangentX)
        {
            Vertices.Add(P);
            Normals.Add(N);
            UVs.Add(UV);
            Colors.Add(Color);
            Tangents.Add(FProcMeshTangent(TangentX, false));
            return Vertices.Num() - 1;
        }

        void AddTriangle(int32 A, int32 B, int32 C, const FVector& FaceNormal)
        {
            const FVector Geo = -FVector::CrossProduct(
                Vertices[B] - Vertices[A], Vertices[C] - Vertices[A]);

            if (FVector::DotProduct(Geo, FaceNormal) < 0.0)
            {
                Swap(B, C);
            }

            Triangles.Add(A);
            Triangles.Add(B);
            Triangles.Add(C);
        }
    };

    void CommitSection(UProceduralMeshComponent* Component, int32 SectionIndex,
        const FMeshData& M, bool bCollision)
    {
        if (!Component || M.IsEmpty()) return;

        Component->CreateMeshSection(
            SectionIndex,
            M.Vertices,
            M.Triangles,
            M.Normals,
            M.UVs,
            M.Colors,
            M.Tangents,
            bCollision);
    }
}

AMapGenerator::AMapGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    DesertMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DesertMesh"));
    SetRootComponent(DesertMesh);
    DesertMesh->bUseAsyncCooking = true;

    WaterMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterMesh"));
    WaterMesh->SetupAttachment(DesertMesh);
    WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaterMesh->SetCastShadow(false);
}

void AMapGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (bAutoGenerate)
    {
        GenerateMap();
    }
}

void AMapGenerator::GenerateMap()
{
    if (!DesertMesh || !WaterMesh) return;

    ClearGenerated();
    ApplyMaterials();

    GridVerts = GridSize + 1;
    BuildTerrainMesh();
    BuildWater();
}

void AMapGenerator::ClearGenerated()
{
    DesertMesh->ClearAllMeshSections();
    WaterMesh->ClearAllMeshSections();

    HeightGrid.Reset();
    BiomeGrid.Reset();
    BlockedGrid.Reset();
    TerrainTint.Reset();

    WaterMesh->SetVisibility(true);
}

void AMapGenerator::ApplyMaterials()
{
    if (DesertMaterial) DesertMesh->SetMaterial(0, DesertMaterial);
    if (WaterMaterial) WaterMesh->SetMaterial(0, WaterMaterial);
}

TArray<FDesertOasis> AMapGenerator::GenerateGlobalOases(float GlobalMapExtent) const
{
    TArray<FDesertOasis> Result;
    FRandomStream Stream(MapSeed);

    for (int32 i = 0; i < OasisCount; ++i)
    {
        FDesertOasis O;
        O.X = Stream.FRandRange(GlobalMapExtent * 0.15f, GlobalMapExtent * (RockTransitionStart - 0.1f));
        O.Y = Stream.FRandRange(GlobalMapExtent * 0.15f, GlobalMapExtent * 0.85f);
        O.Radius = OasisRadius;
        O.ShapeX = Stream.FRandRange(-500.0f, 500.0f);
        O.ShapeY = Stream.FRandRange(-500.0f, 500.0f);
        Result.Add(O);
    }

    return Result;
}

float AMapGenerator::OasisNormalizedDistance(float X, float Y, const TArray<FDesertOasis>& OasesList) const
{
    float Best = TNumericLimits<float>::Max();

    for (const FDesertOasis& O : OasesList)
    {
        const float DX = X - O.X;
        const float DY = Y - O.Y;
        const float Dist2 = DX * DX + DY * DY;

        const float Reject = O.Radius * 6.0f;
        if (Dist2 > Reject * Reject) continue;

        const float Angle = FMath::Atan2(DY, DX);
        const float Dist = FMath::Sqrt(Dist2);

        Best = FMath::Min(Best, Dist / (O.Radius * OasisShapeFactor(O, Angle)));
    }

    return Best;
}

void AMapGenerator::ComputeBiomeWeights(float X, float Y, float GlobalMapExtent, float* OutW) const
{
    OutW[0] = 0.0f; // Desert
    OutW[1] = 0.0f; // Oasis
    OutW[2] = 0.0f; // Rocky

    const float NormX = FMath::Clamp(X / GlobalMapExtent, 0.0f, 1.0f);
    
    const float RockyFactor = Smooth01((NormX - RockTransitionStart) / FMath::Max(RockTransitionWidth, 0.01f));
    const float DesertFactor = 1.0f - RockyFactor;

    OutW[0] = DesertFactor;
    OutW[2] = RockyFactor;
}

float AMapGenerator::ComputeHeight(float X, float Y, float GlobalMapExtent, const float* W, float OasisDist) const
{
    const float WDesert = W[0];
    const float WRocky = W[2];

    const float Cx = X / CellSize;
    const float Cy = Y / CellSize;

    const float LargeNoise = FMath::PerlinNoise2D(FVector2D(Cx * NoiseScale + 12.3f, Cy * NoiseScale + 45.6f));
    const float DetailNoise = FMath::PerlinNoise2D(FVector2D(Cx * DetailScale + 78.9f, Cy * DetailScale + 32.1f));

    float Height = (LargeNoise + DetailNoise * DetailStrength) * HeightScale;
    Height *= WDesert + WRocky * RockyHeightMultiplier;

    if (DuneHeight > 0.0f && WDesert > 0.01f)
    {
        const float Angle = FMath::DegreesToRadians(DuneWindAngle);
        const float CosA = FMath::Cos(Angle);
        const float SinA = FMath::Sin(Angle);

        const float Along = Cx * CosA + Cy * SinA;
        float Across = -Cx * SinA + Cy * CosA;

        Across += 3.0f * FMath::PerlinNoise2D(FVector2D(Along * 0.04f + 11.3f, Across * 0.04f + 5.7f));

        const float Ridge = 1.0f - FMath::Abs(FMath::PerlinNoise2D(FVector2D(Across * DuneFrequency + 100.0f, Along * DuneFrequency * DuneStretch + 200.0f)));
        Height += (Ridge * Ridge - 0.35f) * DuneHeight * WDesert;
    }

    if (WRocky > 0.01f)
    {
        const float GorgeNoise1 = FMath::Abs(FMath::PerlinNoise2D(FVector2D(Cx * 0.004f + 33.0f, Cy * 0.004f + 77.0f)));
        const float GorgeNoise2 = FMath::Abs(FMath::PerlinNoise2D(FVector2D(Cx * 0.012f + 15.0f, Cy * 0.012f + 44.0f)));
        
        float CanyonShape = FMath::Pow(1.0f - GorgeNoise1, 3.5f) * 1.3f + GorgeNoise2 * 0.25f;
        float CanyonCut = CanyonShape * CanyonDepth;
        
        Height -= CanyonCut * WRocky;

        if (TerraceStep > 1.0f)
        {
            const float S = Height / TerraceStep;
            const float F = FMath::FloorToFloat(S);
            const float Terraced = (F + Smooth01((S - F - 0.2f) / 0.12f)) * TerraceStep;
            Height = FMath::Lerp(Height, Terraced, WRocky * 0.95f);
        }
    }

    // --- CUVETTE D'OASIS ÉLARGIE ET PROFONDE ---
    if (OasisDist < MaxOasisInfluence)
    {
        // Pente d'approche plus douce et large
        const float Plain = 1.0f - Smooth01((OasisDist - 2.8f) / 1.5f);
        Height = FMath::Lerp(Height, 0.0f, Plain);

        // Cuvette beaucoup plus large et profonde
        const float Bowl = 1.0f - Smooth01((OasisDist - 0.5f) / 1.8f);
        Height = FMath::Lerp(Height, -OasisDepth, Bowl);
    }

    return Height;
}

float AMapGenerator::SampleHeightAtWorld(float WorldX, float WorldY, float GlobalMapExtent) const
{
    float W[4];
    ComputeBiomeWeights(WorldX, WorldY, GlobalMapExtent, W);
    TArray<FDesertOasis> OasesList = GenerateGlobalOases(GlobalMapExtent);
    float OasisDist = OasisNormalizedDistance(WorldX, WorldY, OasesList);
    return ComputeHeight(WorldX, WorldY, GlobalMapExtent, W, OasisDist);
}

FVector AMapGenerator::SampleNormalAtWorld(float WorldX, float WorldY, float GlobalMapExtent) const
{
    const float E = CellSize * 0.5f;
    const float Dx = (SampleHeightAtWorld(WorldX + E, WorldY, GlobalMapExtent) - SampleHeightAtWorld(WorldX - E, WorldY, GlobalMapExtent)) / (2.0f * E);
    const float Dy = (SampleHeightAtWorld(WorldX, WorldY + E, GlobalMapExtent) - SampleHeightAtWorld(WorldX, WorldY - E, GlobalMapExtent)) / (2.0f * E);
    return FVector(-Dx, -Dy, 1.0f).GetSafeNormal();
}

float AMapGenerator::GetTerrainHeightAtWorld(float WorldX, float WorldY) const
{
    const float GlobalMapExtent = GridSize * CellSize * 2.0f;
    const FTransform& T = GetActorTransform();
    const FVector Local = T.InverseTransformPosition(FVector(WorldX, WorldY, GetActorLocation().Z));
    
    float OffX = 0.0f, OffY = 0.0f;
    if (Quadrant == EMapQuadrant::TopRight || Quadrant == EMapQuadrant::BottomRight) OffX = GridSize * CellSize;
    if (Quadrant == EMapQuadrant::BottomLeft || Quadrant == EMapQuadrant::BottomRight) OffY = GridSize * CellSize;

    const float LocalH = SampleHeightAtWorld(OffX + Local.X, OffY + Local.Y, GlobalMapExtent);
    return T.TransformPosition(FVector(Local.X, Local.Y, LocalH)).Z;
}

void AMapGenerator::BuildTerrainMesh()
{
    FMeshData Mesh;
    const int32 Count = GridVerts * GridVerts;
    Mesh.Vertices.Reserve(Count);
    Mesh.Normals.Reserve(Count);
    Mesh.UVs.Reserve(Count);
    Mesh.Colors.Reserve(Count);
    Mesh.Tangents.Reserve(Count);
    Mesh.Triangles.Reserve(GridSize * GridSize * 6);

    const float GlobalMapExtent = GridSize * CellSize * 2.0f;
    TArray<FDesertOasis> OasesList = GenerateGlobalOases(GlobalMapExtent);

    float QuadrantOffsetX = 0.0f;
    float QuadrantOffsetY = 0.0f;

    if (Quadrant == EMapQuadrant::TopRight || Quadrant == EMapQuadrant::BottomRight)
    {
        QuadrantOffsetX = GridSize * CellSize;
    }
    if (Quadrant == EMapQuadrant::BottomLeft || Quadrant == EMapQuadrant::BottomRight)
    {
        QuadrantOffsetY = GridSize * CellSize;
    }

    const FLinearColor SandTint(0.82f, 0.66f, 0.40f);
    const FLinearColor RockTint(0.52f, 0.32f, 0.22f);
    const FLinearColor GrassTint(0.36f, 0.48f, 0.20f);
    const FLinearColor WetTint(0.30f, 0.26f, 0.17f);
    const FLinearColor SteepRockTint(0.38f, 0.22f, 0.16f);

    HeightGrid.SetNumUninitialized(Count);
    BiomeGrid.Init(EDesertBiome::Desert, Count);
    BlockedGrid.Init(0, Count);
    TerrainTint.SetNumUninitialized(Count);

    for (int32 Y = 0; Y < GridVerts; ++Y)
    {
        for (int32 X = 0; X < GridVerts; ++X)
        {
            const int32 Idx = Y * GridVerts + X;
            const float WX = QuadrantOffsetX + (X * CellSize);
            const float WY = QuadrantOffsetY + (Y * CellSize);

            float W[4];
            ComputeBiomeWeights(WX, WY, GlobalMapExtent, W);

            const float OasisDist = OasisNormalizedDistance(WX, WY, OasesList);
            const float H = ComputeHeight(WX, WY, GlobalMapExtent, W, OasisDist);
            HeightGrid[Idx] = H;

            EDesertBiome Biome = (W[2] > 0.5f) ? EDesertBiome::Rocky : EDesertBiome::Desert;
            if (OasisDist < MaxOasisInfluence)
            {
                Biome = EDesertBiome::Oasis;
                BlockedGrid[Idx] = 1;
            }
            BiomeGrid[Idx] = Biome;

            const FVector Normal = SampleNormalAtWorld(WX, WY, GlobalMapExtent);
            const FVector TangentX = FVector(1.0f, 0.0f, Normal.Z).GetSafeNormal();

            const float Slope = Smooth01((1.0f - static_cast<float>(Normal.Z) - 0.08f) / 0.25f);
            FLinearColor Tint = SandTint * W[0] + RockTint * W[2];

            if (OasisDist < MaxOasisInfluence)
            {
                const float Grass = 1.0f - Smooth01((OasisDist - 1.5f) / 2.0f);
                Tint = LerpColor(Tint, GrassTint, Grass * 0.8f);

                const float Wet = 1.0f - Smooth01((OasisDist - 1.0f) / 0.8f);
                Tint = LerpColor(Tint, WetTint, Wet);
            }

            Tint = LerpColor(Tint, SteepRockTint, Slope);
            const float Variation = 1.0f + 0.07f * FMath::PerlinNoise2D(FVector2D(WX * 0.0035f + 10.0f, WY * 0.0035f + 20.0f));
            Tint = Tint * Variation;
            Tint.A = 1.0f;
            TerrainTint[Idx] = Tint;

            Mesh.AddVertex(
                FVector(X * CellSize, Y * CellSize, H),
                Normal,
                FVector2D(WX / TerrainUVWorldSize, WY / TerrainUVWorldSize),
                ToVertexColor(Tint),
                TangentX);
        }
    }

    for (int32 Y = 0; Y < GridSize; ++Y)
    {
        for (int32 X = 0; X < GridSize; ++X)
        {
            const int32 A = Y * GridVerts + X;
            const int32 B = A + 1;
            const int32 C = A + GridVerts;
            const int32 D = C + 1;

            Mesh.Triangles.Add(A);
            Mesh.Triangles.Add(C);
            Mesh.Triangles.Add(B);

            Mesh.Triangles.Add(B);
            Mesh.Triangles.Add(C);
            Mesh.Triangles.Add(D);
        }
    }

    CommitSection(DesertMesh, 0, Mesh, true);
}

void AMapGenerator::BuildWater()
{
    FMeshData Mesh;
    const float WaterZ = -OasisDepth * 0.70f; // Ajusté pour que l'eau se pose bien au fond de la grande cuvette
    const FVector Up(0.0f, 0.0f, 1.0f);
    const FVector TangentX(1.0f, 0.0f, 0.0f);

    const float GlobalMapExtent = GridSize * CellSize * 2.0f;
    TArray<FDesertOasis> OasesList = GenerateGlobalOases(GlobalMapExtent);

    float QuadrantOffsetX = (Quadrant == EMapQuadrant::TopRight || Quadrant == EMapQuadrant::BottomRight) ? GridSize * CellSize : 0.0f;
    float QuadrantOffsetY = (Quadrant == EMapQuadrant::BottomLeft || Quadrant == EMapQuadrant::BottomRight) ? GridSize * CellSize : 0.0f;

    for (const FDesertOasis& O : OasesList)
    {
        const float LocalOX = O.X - QuadrantOffsetX;
        const float LocalOY = O.Y - QuadrantOffsetY;
        const float MaxRadiusCheck = O.Radius * WaterRadiusRatio * 1.5f;

        if (LocalOX < -MaxRadiusCheck || LocalOX >(GridSize * CellSize + MaxRadiusCheck) ||
            LocalOY < -MaxRadiusCheck || LocalOY >(GridSize * CellSize + MaxRadiusCheck))
        {
            continue;
        }

        const int32 Center = Mesh.AddVertex(
            FVector(LocalOX, LocalOY, WaterZ), Up, FVector2D(0.5f, 0.5f),
            FColor(255, 255, 255, 255), TangentX);

        int32 FirstVert = -1;
        int32 PrevVert = -1;

        for (int32 I = 0; I < WaterSegments; ++I)
        {
            const float Angle = 2.0f * PI * static_cast<float>(I) / WaterSegments;
            const float CosA = FMath::Cos(Angle);
            const float SinA = FMath::Sin(Angle);

            const float Radius = O.Radius * WaterRadiusRatio * OasisShapeFactor(O, Angle);
            const float VX = LocalOX + CosA * Radius;
            const float VY = LocalOY + SinA * Radius;

            const int32 V = Mesh.AddVertex(
                FVector(VX, VY, WaterZ),
                Up,
                FVector2D(0.5f + CosA * 0.5f, 0.5f + SinA * 0.5f),
                FColor(255, 255, 255, 0),
                TangentX);

            if (I == 0) FirstVert = V;
            if (PrevVert != -1)
            {
                Mesh.AddTriangle(Center, PrevVert, V, Up);
            }
            PrevVert = V;
        }
        Mesh.AddTriangle(Center, PrevVert, FirstVert, Up);
    }

    CommitSection(WaterMesh, 0, Mesh, false);
}