
#include "MapGenerator.h"

#include "KismetProceduralMeshLibrary.h"

AMapGenerator::AMapGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    DesertMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DesertMesh"));
    SetRootComponent(DesertMesh);

    WaterMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterMesh"));
    WaterMesh->SetupAttachment(DesertMesh);
    WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaterMesh->SetCastShadow(false);

    RoadMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RoadMesh"));
    RoadMesh->SetupAttachment(DesertMesh);

    CityMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CityMesh"));
    CityMesh->SetupAttachment(DesertMesh);

    TreeInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TreeInstances"));
    TreeInstances->SetupAttachment(DesertMesh);

    RockInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RockInstances"));
    RockInstances->SetupAttachment(DesertMesh);

    DesertMesh->bUseAsyncCooking = true;
}

void AMapGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    GenerateDesert();
}

void AMapGenerator::GenerateDesert()
{
    if (!DesertMesh || !WaterMesh || !RoadMesh || !CityMesh ||
        !TreeInstances || !RockInstances)
    {
        return;
    }

    // Nettoyage des meshes et des instances précédentes.
    DesertMesh->ClearAllMeshSections();
    WaterMesh->ClearAllMeshSections();
    RoadMesh->ClearAllMeshSections();
    CityMesh->ClearAllMeshSections();

    TreeInstances->ClearInstances();
    RockInstances->ClearInstances();

    TreeInstances->SetStaticMesh(TreeMesh);
    RockInstances->SetStaticMesh(RockMesh);

    if (DesertMaterial)
        DesertMesh->SetMaterial(0, DesertMaterial);

    if (WaterMaterial)
        WaterMesh->SetMaterial(0, WaterMaterial);

    if (RoadMaterial)
        RoadMesh->SetMaterial(0, RoadMaterial);

    if (BuildingMaterial)
        CityMesh->SetMaterial(0, BuildingMaterial);

    WaterMesh->SetVisibility(true);

    const int32 Size = FMath::Clamp(GridSize, 2, 500);
    const float Extent = Size * CellSize;
    const float CityHalfSize = Extent * CitySizeRatio;

    const FVector2D CityCenter(Extent * 0.5f, Extent * 0.5f);

    // ---------------------------------------------------------
    // 1. GÉNÉRATION DES RÉGIONS ET DES BIOMES
    // ---------------------------------------------------------

    struct FBiomeRegion
    {
        FVector2D Center;
        EDesertBiome Type;
    };

    TArray<FBiomeRegion> Regions;

    FRandomStream RegionRandom(Seed);

    const int32 RegionCount = FMath::Clamp(BiomeRegionCount, 2, 10);
    const float RegionSpacing = Extent / RegionCount;
    const float Jitter = RegionSpacing * 0.25f;

    const float RegionNoiseX = RegionRandom.FRandRange(-1000.0f, 1000.0f);
    const float RegionNoiseY = RegionRandom.FRandRange(-1000.0f, 1000.0f);

    for (int32 Y = 0; Y < RegionCount; ++Y)
    {
        for (int32 X = 0; X < RegionCount; ++X)
        {
            const float CenterX = (X + 0.5f) * RegionSpacing
                + RegionRandom.FRandRange(-Jitter, Jitter);

            const float CenterY = (Y + 0.5f) * RegionSpacing
                + RegionRandom.FRandRange(-Jitter, Jitter);

            EDesertBiome Type = EDesertBiome::Desert;

            const int32 Index = Y * RegionCount + X;

            if (Index == 0)
            {
                Type = EDesertBiome::Oasis;
            }
            else if (Index == 1)
            {
                Type = EDesertBiome::Rocky;
            }
            else if (Index == 2)
            {
                Type = EDesertBiome::Forest;
            }
            else
            {
                const float Choice = RegionRandom.FRand();

                if (Choice < OasisDensity)
                {
                    Type = EDesertBiome::Oasis;
                }
                else if (Choice < OasisDensity + RockDensity)
                {
                    Type = EDesertBiome::Rocky;
                }
                else if (Choice < OasisDensity + RockDensity + ForestDensity)
                {
                    Type = EDesertBiome::Forest;
                }
            }

            FBiomeRegion Region;
            Region.Center = FVector2D(CenterX, CenterY);
            Region.Type = Type;

            Regions.Add(Region);
        }
    }

    // ---------------------------------------------------------
    // 2. DÉTERMINATION DU BIOME À UNE POSITION
    // ---------------------------------------------------------

    auto FindRegionIndex = [&](float X, float Y) -> int32
        {
            int32 BestIndex = 0;
            float BestScore = TNumericLimits<float>::Max();

            for (int32 I = 0; I < Regions.Num(); ++I)
            {
                const FVector2D Delta =
                    FVector2D(X, Y) - Regions[I].Center;

                const float Distance = Delta.Size();

                const float BoundaryNoise = FMath::PerlinNoise2D(
                    FVector2D(
                        (X + Regions[I].Center.X * 0.37f)
                        * 0.001f + RegionNoiseX,
                        (Y + Regions[I].Center.Y * 0.37f)
                        * 0.001f + RegionNoiseY));

                const float Score =
                    Distance + BoundaryNoise * RegionSpacing * BiomeVariation;

                if (Score < BestScore)
                {
                    BestScore = Score;
                    BestIndex = I;
                }
            }

            return BestIndex;
        };

    auto GetBiomeAt = [&](float X, float Y) -> EDesertBiome
        {
            if (bGenerateCity &&
                (FVector2D(X, Y) - CityCenter).Size() < CityHalfSize)
            {
                return EDesertBiome::City;
            }

            return Regions[FindRegionIndex(X, Y)].Type;
        };

    // ---------------------------------------------------------
    // 3. HAUTEUR DU TERRAIN
    // ---------------------------------------------------------

    FRandomStream TerrainRandom(Seed);

    const float TerrainOffsetX =
        TerrainRandom.FRandRange(-100000.0f, 100000.0f);

    const float TerrainOffsetY =
        TerrainRandom.FRandRange(-100000.0f, 100000.0f);

    auto GetHeightAt = [&](float X, float Y) -> float
        {
            const float LargeNoise = FMath::PerlinNoise2D(
                FVector2D(
                    X / CellSize * NoiseScale + TerrainOffsetX,
                    Y / CellSize * NoiseScale + TerrainOffsetY));

            const float DetailNoise = FMath::PerlinNoise2D(
                FVector2D(
                    X / CellSize * DetailScale + TerrainOffsetX + 543.0f,
                    Y / CellSize * DetailScale + TerrainOffsetY + 987.0f));

            float Height =
                (LargeNoise + DetailNoise * DetailStrength) * HeightScale;

            // Aplanissement progressif de la zone urbaine.
            if (bGenerateCity)
            {
                const float Distance =
                    (FVector2D(X, Y) - CityCenter).Size();

                const float Start = CityHalfSize * 0.75f;
                const float End = CityHalfSize * 1.25f;

                float T = FMath::Clamp(
                    (Distance - Start) /
                    FMath::Max(End - Start, 1.0f),
                    0.0f, 1.0f);

                T = T * T * (3.0f - 2.0f * T);

                const float CityBlend = 1.0f - T;

                Height = FMath::Lerp(Height, 0.0f, CityBlend);
            }

            // Création d'une cuvette pour les oasis.
            const int32 RegionIndex = FindRegionIndex(X, Y);
            const FBiomeRegion& Region = Regions[RegionIndex];

            if (Region.Type == EDesertBiome::Oasis)
            {
                const float Distance =
                    (FVector2D(X, Y) - Region.Center).Size();

                const float OasisRadius = RegionSpacing * 0.14f;
                const float Start = OasisRadius * 0.7f;
                const float End = OasisRadius * 1.8f;

                float T = FMath::Clamp(
                    (Distance - Start) /
                    FMath::Max(End - Start, 1.0f),
                    0.0f, 1.0f);

                T = T * T * (3.0f - 2.0f * T);

                const float OasisBlend = 1.0f - T;

                Height = FMath::Lerp(Height, -20.0f, OasisBlend);
            }

            return Height;
        };

    // ---------------------------------------------------------
    // 4. OUTILS POUR CONSTRUIRE LES MESHES
    // ---------------------------------------------------------

    auto AppendQuad = [](
        TArray<FVector>& V,
        TArray<int32>& T,
        TArray<FVector2D>& UV,
        const FVector& P0,
        const FVector& P1,
        const FVector& P2,
        const FVector& P3)
        {
            const int32 Start = V.Num();

            V.Add(P0);
            V.Add(P1);
            V.Add(P2);
            V.Add(P3);

            UV.Add(FVector2D(0.0f, 0.0f));
            UV.Add(FVector2D(1.0f, 0.0f));
            UV.Add(FVector2D(1.0f, 1.0f));
            UV.Add(FVector2D(0.0f, 1.0f));

            T.Add(Start);
            T.Add(Start + 1);
            T.Add(Start + 2);

            T.Add(Start);
            T.Add(Start + 2);
            T.Add(Start + 3);
        };

    auto AppendBox = [&](
        TArray<FVector>& V,
        TArray<int32>& T,
        TArray<FVector2D>& UV,
        float X,
        float Y,
        float HalfX,
        float HalfY,
        float BaseZ,
        float Height)
        {
            const float X0 = X - HalfX;
            const float X1 = X + HalfX;
            const float Y0 = Y - HalfY;
            const float Y1 = Y + HalfY;
            const float Z0 = BaseZ;
            const float Z1 = BaseZ + Height;

            // Dessus.
            AppendQuad(V, T, UV,
                FVector(X0, Y0, Z1),
                FVector(X1, Y0, Z1),
                FVector(X1, Y1, Z1),
                FVector(X0, Y1, Z1));

            // Dessous.
            AppendQuad(V, T, UV,
                FVector(X0, Y1, Z0),
                FVector(X1, Y1, Z0),
                FVector(X1, Y0, Z0),
                FVector(X0, Y0, Z0));

            // Face avant.
            AppendQuad(V, T, UV,
                FVector(X0, Y0, Z0),
                FVector(X1, Y0, Z0),
                FVector(X1, Y0, Z1),
                FVector(X0, Y0, Z1));

            // Face arrière.
            AppendQuad(V, T, UV,
                FVector(X1, Y1, Z0),
                FVector(X0, Y1, Z0),
                FVector(X0, Y1, Z1),
                FVector(X1, Y1, Z1));

            // Côté gauche.
            AppendQuad(V, T, UV,
                FVector(X0, Y1, Z0),
                FVector(X0, Y0, Z0),
                FVector(X0, Y0, Z1),
                FVector(X0, Y1, Z1));

            // Côté droit.
            AppendQuad(V, T, UV,
                FVector(X1, Y0, Z0),
                FVector(X1, Y1, Z0),
                FVector(X1, Y1, Z1),
                FVector(X1, Y0, Z1));
        };

    auto BuildMeshSection = [](
        UProceduralMeshComponent* Component,
        const TArray<FVector>& V,
        const TArray<int32>& T,
        const TArray<FVector2D>& UV,
        bool bCollision)
        {
            if (!Component || V.IsEmpty() || T.IsEmpty())
            {
                return;
            }

            TArray<FVector> Normals;
            TArray<FProcMeshTangent> Tangents;

            UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
                V, T, UV, Normals, Tangents);

            Component->CreateMeshSection(
                0,
                V,
                T,
                Normals,
                UV,
                TArray<FColor>(),
                Tangents,
                bCollision);
        };

    // ---------------------------------------------------------
    // 5. GÉNÉRATION DU TERRAIN
    // ---------------------------------------------------------

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector2D> UVs;

    const int32 VertexCount = (Size + 1) * (Size + 1);

    Vertices.Reserve(VertexCount);
    UVs.Reserve(VertexCount);
    Triangles.Reserve(Size * Size * 6);

    for (int32 Y = 0; Y <= Size; ++Y)
    {
        for (int32 X = 0; X <= Size; ++X)
        {
            const float WorldX = X * CellSize;
            const float WorldY = Y * CellSize;

            Vertices.Add(
                FVector(WorldX, WorldY, GetHeightAt(WorldX, WorldY)));

            UVs.Add(FVector2D(
                static_cast<float>(X) / Size,
                static_cast<float>(Y) / Size));
        }
    }

    for (int32 Y = 0; Y < Size; ++Y)
    {
        for (int32 X = 0; X < Size; ++X)
        {
            const int32 A = Y * (Size + 1) + X;
            const int32 B = A + 1;
            const int32 C = A + Size + 1;
            const int32 D = C + 1;

            // Conserve l'ordre de triangles qui fonctionne sur ton terrain.
            Triangles.Add(A);
            Triangles.Add(C);
            Triangles.Add(B);

            Triangles.Add(B);
            Triangles.Add(C);
            Triangles.Add(D);
        }
    }

    BuildMeshSection(DesertMesh, Vertices, Triangles, UVs, true);

    // ---------------------------------------------------------
    // 6. EAU DES OASIS
    // ---------------------------------------------------------

    TArray<FVector> WaterVertices;
    TArray<int32> WaterTriangles;
    TArray<FVector2D> WaterUVs;

    constexpr int32 WaterSegments = 32;

    for (const FBiomeRegion& Region : Regions)
    {
        if (Region.Type != EDesertBiome::Oasis)
        {
            continue;
        }

        if (bGenerateCity &&
            (Region.Center - CityCenter).Size() < CityHalfSize * 1.5f)
        {
            continue;
        }

        // Le rayon reste à l'intérieur de la partie plate de l'oasis.
        const float Radius = RegionSpacing * 0.08f;
        const float WaterZ = -15.0f;

        const int32 CenterIndex = WaterVertices.Num();

        WaterVertices.Add(
            FVector(Region.Center.X, Region.Center.Y, WaterZ));

        WaterUVs.Add(FVector2D(0.5f, 0.5f));

        for (int32 I = 0; I < WaterSegments; ++I)
        {
            const float Angle =
                2.0f * PI * static_cast<float>(I) / WaterSegments;

            const float CosAngle = FMath::Cos(Angle);
            const float SinAngle = FMath::Sin(Angle);

            const float X = Region.Center.X + CosAngle * Radius;
            const float Y = Region.Center.Y + SinAngle * Radius;

            WaterVertices.Add(FVector(X, Y, WaterZ));

            WaterUVs.Add(FVector2D(
                0.5f + CosAngle * 0.5f,
                0.5f + SinAngle * 0.5f));
        }

        for (int32 I = 0; I < WaterSegments; ++I)
        {
            const int32 Center = CenterIndex;
            const int32 B = CenterIndex + 1 + I;
            const int32 C =
                CenterIndex + 1 + ((I + 1) % WaterSegments);

            // Même orientation que celle du terrain corrigé.
            WaterTriangles.Add(Center);
            WaterTriangles.Add(C);
            WaterTriangles.Add(B);
        }
    }

    BuildMeshSection(
        WaterMesh,
        WaterVertices,
        WaterTriangles,
        WaterUVs,
        false);

    WaterMesh->SetVisibility(true);

    // ---------------------------------------------------------
    // 7. VILLE : ROUTES ET BÂTIMENTS
    // ---------------------------------------------------------

    if (bGenerateCity)
    {
        TArray<FVector> RoadVertices;
        TArray<int32> RoadTriangles;
        TArray<FVector2D> RoadUVs;

        TArray<FVector> BuildingVertices;
        TArray<int32> BuildingTriangles;
        TArray<FVector2D> BuildingUVs;

        const float Half = CityHalfSize;
        const float Spacing = FMath::Max(StreetSpacing, 1.0f);
        const float RoadHalfWidth = StreetWidth * 0.5f;

        // Routes en grille.
        for (float Offset = -Half;
            Offset <= Half + 0.01f;
            Offset += Spacing)
        {
            const float X = CityCenter.X + Offset;
            const float Y = CityCenter.Y + Offset;
            const float Z = 5.0f;

            // Route verticale.
            AppendQuad(
                RoadVertices, RoadTriangles, RoadUVs,
                FVector(X - RoadHalfWidth, CityCenter.Y - Half, Z),
                FVector(X + RoadHalfWidth, CityCenter.Y - Half, Z),
                FVector(X + RoadHalfWidth, CityCenter.Y + Half, Z),
                FVector(X - RoadHalfWidth, CityCenter.Y + Half, Z));

            // Route horizontale.
            AppendQuad(
                RoadVertices, RoadTriangles, RoadUVs,
                FVector(CityCenter.X - Half, Y - RoadHalfWidth, Z),
                FVector(CityCenter.X + Half, Y - RoadHalfWidth, Z),
                FVector(CityCenter.X + Half, Y + RoadHalfWidth, Z),
                FVector(CityCenter.X - Half, Y + RoadHalfWidth, Z));
        }

        // Bâtiments avec des proportions et des hauteurs variables.
        FRandomStream CityRandom(Seed + 991);

        const float BuildingBaseZ = 15.0f;

        const float BuildingHalfSize = FMath::Max(
            100.0f,
            Spacing * 0.30f - StreetWidth * 0.5f);

        for (float X = -Half + Spacing * 0.5f;
            X < Half;
            X += Spacing)
        {
            for (float Y = -Half + Spacing * 0.5f;
                Y < Half;
                Y += Spacing)
            {
                // Espaces libres : cours, petites places ou parcelles vides.
                if (CityRandom.FRand() < 0.12f)
                {
                    continue;
                }

                const float BuildingOffsetX = CityRandom.FRandRange(
                    -Spacing * 0.08f, Spacing * 0.08f);

                const float BuildingOffsetY = CityRandom.FRandRange(
                    -Spacing * 0.08f, Spacing * 0.08f);

                const float BuildingX =
                    CityCenter.X + X + BuildingOffsetX;

                const float BuildingY =
                    CityCenter.Y + Y + BuildingOffsetY;

                const float Height = CityRandom.FRandRange(
                    MinBuildingHeight,
                    FMath::Max(MinBuildingHeight, MaxBuildingHeight));

                // Largeur et profondeur différentes pour éviter
                // que tous les bâtiments aient la même silhouette.
                const float HalfWidthX = CityRandom.FRandRange(
                    BuildingHalfSize * 0.45f,
                    BuildingHalfSize);

                const float HalfWidthY = CityRandom.FRandRange(
                    BuildingHalfSize * 0.45f,
                    BuildingHalfSize);

                AppendBox(
                    BuildingVertices,
                    BuildingTriangles,
                    BuildingUVs,
                    BuildingX,
                    BuildingY,
                    HalfWidthX,
                    HalfWidthY,
                    BuildingBaseZ,
                    Height);

                // Certains bâtiments possèdent un étage supérieur
                // en retrait.
                if (CityRandom.FRand() < 0.35f && Height > 300.0f)
                {
                    const float UpperHeight = Height * 0.25f;

                    AppendBox(
                        BuildingVertices,
                        BuildingTriangles,
                        BuildingUVs,
                        BuildingX,
                        BuildingY,
                        HalfWidthX * 0.65f,
                        HalfWidthY * 0.65f,
                        BuildingBaseZ + Height,
                        UpperHeight);
                }

                // Quelques bâtiments possèdent une petite tour sur le toit.
                if (CityRandom.FRand() < 0.15f)
                {
                    const float TowerWidth =
                        FMath::Min(HalfWidthX, HalfWidthY) * 0.35f;

                    const float TowerHeight =
                        CityRandom.FRandRange(80.0f, 180.0f);

                    AppendBox(
                        BuildingVertices,
                        BuildingTriangles,
                        BuildingUVs,
                        BuildingX,
                        BuildingY,
                        TowerWidth,
                        TowerWidth,
                        BuildingBaseZ + Height,
                        TowerHeight);
                }
            }
        }

        BuildMeshSection(
            RoadMesh,
            RoadVertices,
            RoadTriangles,
            RoadUVs,
            true);

        BuildMeshSection(
            CityMesh,
            BuildingVertices,
            BuildingTriangles,
            BuildingUVs,
            true);
    }

    // ---------------------------------------------------------
    // 8. VÉGÉTATION ET ROCHERS
    // ---------------------------------------------------------

    if (TreeMesh || RockMesh)
    {
        FRandomStream InstanceRandom(Seed + 4242);

        const int32 Step = FMath::Clamp(InstanceSampleStep, 1, 20);

        for (int32 Y = 0; Y < Size; Y += Step)
        {
            for (int32 X = 0; X < Size; X += Step)
            {
                const float WorldX =
                    (X + InstanceRandom.FRand()) * CellSize;

                const float WorldY =
                    (Y + InstanceRandom.FRand()) * CellSize;

                const EDesertBiome Biome = GetBiomeAt(WorldX, WorldY);

                if (Biome == EDesertBiome::City)
                {
                    continue;
                }

                const float Height = GetHeightAt(WorldX, WorldY);
                const float Roll = InstanceRandom.FRand();

                float TreeChance = 0.0f;

                if (Biome == EDesertBiome::Forest)
                {
                    TreeChance = 0.55f;
                }
                else if (Biome == EDesertBiome::Oasis)
                {
                    TreeChance = 0.20f;
                }
                else if (Biome == EDesertBiome::Desert)
                {
                    TreeChance = 0.015f;
                }

                if (TreeMesh && Roll < TreeChance)
                {
                    const float Scale =
                        InstanceRandom.FRandRange(0.8f, 1.4f);

                    const float Yaw =
                        InstanceRandom.FRandRange(0.0f, 360.0f);

                    TreeInstances->AddInstance(FTransform(
                        FRotator(0.0f, Yaw, 0.0f),
                        FVector(WorldX, WorldY, Height),
                        FVector(Scale)));
                }

                float RockChance = 0.0f;

                if (Biome == EDesertBiome::Rocky)
                {
                    RockChance = 0.30f;
                }
                else if (Biome == EDesertBiome::Desert)
                {
                    RockChance = 0.08f;
                }
                else if (Biome == EDesertBiome::Forest)
                {
                    RockChance = 0.03f;
                }

                if (RockMesh && InstanceRandom.FRand() < RockChance)
                {
                    const float Scale =
                        InstanceRandom.FRandRange(0.6f, 1.8f);

                    const float Yaw =
                        InstanceRandom.FRandRange(0.0f, 360.0f);

                    RockInstances->AddInstance(FTransform(
                        FRotator(0.0f, Yaw, 0.0f),
                        FVector(WorldX, WorldY, Height),
                        FVector(Scale)));
                }
            }
        }
    }
}
