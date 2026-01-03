#include "World/TerraDyneChunk.h" // MUST BE FIRST INCLUDE
#include "IO/TerraDyneAsyncSaver.h"

// Engine Includes
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshSpatialFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DynamicMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Async/Async.h"

// TerraDyne Includes
#include "Core/TerraDyneSubsystem.h"
#include "Grass/TerraDyneGrassSystem.h"
#include "Core/TerraDyneManager.h"

// Helper Macros
#define GRID_INDEX(X, Y) ((Y) * Resolution + (X))

ATerraDyneChunk::ATerraDyneChunk()
{
	PrimaryActorTick.bCanEverTick = false;
	// Disable spatial loading for demo stability
	bIsSpatiallyLoaded = false;

	PhysicsMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("PhysicsMesh"));
	SetRootComponent(PhysicsMesh);

	PhysicsMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PhysicsMesh->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
	PhysicsMesh->SetGenerateOverlapEvents(false);

	VisualMesh = CreateDefaultSubobject<UVirtualHeightfieldMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);

	GridCoordinate = FIntPoint(0, 0);
	ChunkSizeWorldUnits = 10000.0f;
	Resolution = 128;
	bPhysicsIsDirty = false;
	CollisionUpdateDelay = 0.1f;
}

UTextureRenderTarget2D* ATerraDyneChunk::CreateInternalRT(int32 Res, ETextureRenderTargetFormat Format, FLinearColor ClearColor)
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	RT->RenderTargetFormat = Format;
	RT->InitAutoFormat(Res, Res);
	RT->ClearColor = ClearColor;
	RT->UpdateResourceImmediate(true);
	return RT;
}

void ATerraDyneChunk::BeginPlay()
{
	Super::BeginPlay();

	// Self Healing: If empty, init default so physics works
	if (HeightCache.Num() == 0 && !LinkedTileData)
	{
		InitializeChunk(GridCoordinate, ChunkSizeWorldUnits, 128, nullptr);
		RebuildPhysicsMesh(); // Vital force build
		UE_LOG(LogTemp, Log, TEXT("TerraDyneChunk: Self-Initialized empty chunk at %s"), *GetActorLocation().ToString());
	}
	else if (LinkedTileData)
	{
		InitializeFromAsset(LinkedTileData);
	}

	// Material Injection from Manager
	if (ATerraDyneManager* Manager = Cast<ATerraDyneManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATerraDyneManager::StaticClass())))
	{
		if (!BrushMaterialBase && Manager->HeightBrushMaterial) BrushMaterialBase = Manager->HeightBrushMaterial;
		if (!PaintMaterialBase && Manager->WeightBrushMaterial) PaintMaterialBase = Manager->WeightBrushMaterial;
	}
}

void ATerraDyneChunk::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_CollisionUpdate);
	Super::EndPlay(EndPlayReason);
}

void ATerraDyneChunk::InitializeFromAsset(UTerraDyneTileData* TileData)
{
	if (!TileData) return;

	Resolution = TileData->Resolution;
	ChunkSizeWorldUnits = TileData->RealWorldSize;

	HeightCache.SetNumUninitialized(TileData->InitialHeightMap.Num());
	ParallelFor(TileData->InitialHeightMap.Num(), [&](int32 i)
		{
			float Normalized = (float)TileData->InitialHeightMap[i] / 65535.0f;
			HeightCache[i] = Normalized * (ZScale * 512.0f);
		});

	HeightRT = CreateInternalRT(Resolution, RTF_R16f, FLinearColor::Black);
	WeightRT = CreateInternalRT(Resolution, RTF_RGBA8, FLinearColor(0, 0, 0, 0));
	UpdateVisualTexture();

	RebuildPhysicsMesh(); // Build collision

	bPhysicsIsDirty = true;
	PerformDeferredCollisionUpdate();
}

void ATerraDyneChunk::InitializeChunk(FIntPoint Coord, float Size, int32 InRes, UTexture2D* SourceHeight, UTexture2D* SourceWeight)
{
	GridCoordinate = Coord;
	ChunkSizeWorldUnits = Size;
	Resolution = InRes;

	HeightCache.SetNumZeroed(Resolution * Resolution);

	HeightRT = CreateInternalRT(Resolution, RTF_R16f, FLinearColor::Black);
	WeightRT = CreateInternalRT(Resolution, RTF_RGBA8, FLinearColor(0, 0, 0, 0));
}

void ATerraDyneChunk::RebuildPhysicsMesh()
{
	if (!PhysicsMesh) return;

	PhysicsMesh->GetDynamicMesh()->Reset();

	FGeometryScriptPrimitiveOptions Options;
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(
		PhysicsMesh->GetDynamicMesh(),
		Options,
		FTransform::Identity,
		ChunkSizeWorldUnits, ChunkSizeWorldUnits,
		Resolution / 2, Resolution / 2
	);

	PhysicsMesh->UpdateCollision(true);
}

void ATerraDyneChunk::ApplyLocalIdempotentEdit(FVector RelativePos, float Radius, float Strength, bool bIsHole, int32 PaintLayer)
{
	if (HeightCache.Num() == 0) return;

	if (PaintLayer >= 0)
	{
		ApplyPaintBrush(GetActorLocation() + RelativePos, Radius, 1.0f, PaintLayer);
		return;
	}

	float HalfSize = ChunkSizeWorldUnits * 0.5f;
	float gridX = ((RelativePos.X + HalfSize) / ChunkSizeWorldUnits) * (Resolution - 1);
	float gridY = ((RelativePos.Y + HalfSize) / ChunkSizeWorldUnits) * (Resolution - 1);
	float radGrid = (Radius / ChunkSizeWorldUnits) * (Resolution - 1);

	int32 minX = FMath::Clamp(FMath::FloorToInt(gridX - radGrid), 0, Resolution - 1);
	int32 maxX = FMath::Clamp(FMath::CeilToInt(gridX + radGrid), 0, Resolution - 1);
	int32 minY = FMath::Clamp(FMath::FloorToInt(gridY - radGrid), 0, Resolution - 1);
	int32 maxY = FMath::Clamp(FMath::CeilToInt(gridY + radGrid), 0, Resolution - 1);

	bool bModified = false;

	if (!bIsHole)
	{
		for (int32 y = minY; y <= maxY; y++)
		{
			for (int32 x = minX; x <= maxX; x++)
			{
				float dist = FVector2D::Distance(FVector2D(x, y), FVector2D(gridX, gridY));
				if (dist <= radGrid)
				{
					float Alpha = 1.0f - (dist / radGrid);
					int32 idx = GRID_INDEX(x, y);
					HeightCache[idx] += Strength * Alpha;
					bModified = true;
				}
			}
		}
	}

	if (!bModified) return;

	// GPU Draw
	if (!TerrainMID && BrushMaterialBase)
	{
		TerrainMID = UMaterialInstanceDynamic::Create(BrushMaterialBase, this);
	}

	if (TerrainMID && HeightRT)
	{
		FVector2D BrushUV(
			(RelativePos.X + HalfSize) / ChunkSizeWorldUnits,
			(RelativePos.Y + HalfSize) / ChunkSizeWorldUnits
		);

		TerrainMID->SetVectorParameterValue(TEXT("BrushPos"), FLinearColor(BrushUV.X, BrushUV.Y, 0, 0));
		TerrainMID->SetScalarParameterValue(TEXT("Radius"), Radius / ChunkSizeWorldUnits);
		TerrainMID->SetScalarParameterValue(TEXT("Strength"), Strength);

		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, HeightRT, TerrainMID);
	}

	bPhysicsIsDirty = true;
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_CollisionUpdate,
		this,
		&ATerraDyneChunk::PerformDeferredCollisionUpdate,
		CollisionUpdateDelay,
		false
	);

	// Vegetation Update
	if (UTerraDyneSubsystem* Subsystem = GetWorld()->GetSubsystem<UTerraDyneSubsystem>())
	{
		if (TSharedPtr<FTerraDyneGrassSystem> GrassSys = Subsystem->GetGrassSystem())
		{
			FBox WorldBounds = FBox(GetActorLocation() + RelativePos - FVector(Radius), GetActorLocation() + RelativePos + FVector(Radius));
			GrassSys->RequestRegen(WorldBounds);
		}
	}
}

void ATerraDyneChunk::ApplyPaintBrush(FVector WorldPos, float Radius, float Strength, int32 LayerChannel)
{
	if (!PaintMaterialBase || !WeightRT) return;

	UMaterialInstanceDynamic* PaintMID = UMaterialInstanceDynamic::Create(PaintMaterialBase, this);

	FVector LocalPos = GetActorTransform().InverseTransformPosition(WorldPos);
	float HalfSize = ChunkSizeWorldUnits * 0.5f;
	FVector2D UV(
		(LocalPos.X + HalfSize) / ChunkSizeWorldUnits,
		(LocalPos.Y + HalfSize) / ChunkSizeWorldUnits
	);

	PaintMID->SetVectorParameterValue(TEXT("BrushPos"), FLinearColor(UV.X, UV.Y, 0, 0));
	PaintMID->SetScalarParameterValue(TEXT("Radius"), Radius / ChunkSizeWorldUnits);
	PaintMID->SetScalarParameterValue(TEXT("Strength"), Strength);

	FLinearColor Mask(
		LayerChannel == 0 ? 1 : 0,
		LayerChannel == 1 ? 1 : 0,
		LayerChannel == 2 ? 1 : 0,
		LayerChannel == 3 ? 1 : 0
	);
	PaintMID->SetVectorParameterValue(TEXT("ChannelMask"), Mask);

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, WeightRT, PaintMID);
}

void ATerraDyneChunk::PerformDeferredCollisionUpdate()
{
	if (!bPhysicsIsDirty) return;
	SyncPhysicsGeometry();
	PhysicsMesh->UpdateCollision(true);
	bPhysicsIsDirty = false;
}

void ATerraDyneChunk::SyncPhysicsGeometry()
{
	PhysicsMesh->GetDynamicMesh()->EditMesh([&](FDynamicMesh3& Mesh)
		{
			for (int32 vid : Mesh.VertexIndicesItr())
			{
				FVector3d Pos = Mesh.GetVertex(vid);

				float HalfSize = ChunkSizeWorldUnits * 0.5f;
				int32 X = FMath::Clamp(FMath::RoundToInt(((Pos.X + HalfSize) / ChunkSizeWorldUnits) * (Resolution - 1)), 0, Resolution - 1);
				int32 Y = FMath::Clamp(FMath::RoundToInt(((Pos.Y + HalfSize) / ChunkSizeWorldUnits) * (Resolution - 1)), 0, Resolution - 1);

				float NewZ = HeightCache[GRID_INDEX(X, Y)];
				Mesh.SetVertex(vid, FVector3d(Pos.X, Pos.Y, NewZ));
			}

		}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::VertexPositions);
}

void ATerraDyneChunk::UpdateVisualTexture()
{
}

void ATerraDyneChunk::SetMaterial(UMaterialInterface* InMaterial)
{
	if (!VisualMesh || !InMaterial) return;

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(InMaterial, this);
	MID->SetTextureParameterValue(TEXT("HeightMap"), HeightRT);
	MID->SetTextureParameterValue(TEXT("WeightMap"), WeightRT);
	MID->SetScalarParameterValue(TEXT("ZScale"), ZScale);

	if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(VisualMesh))
	{
		PrimComp->SetMaterial(0, MID);
	}
}

void ATerraDyneChunk::SaveAsync(FString SlotName)
{
	FTerraDyneChunkSnapshot Snapshot;
	Snapshot.GridCoordinate = GridCoordinate;
	Snapshot.HeightData = HeightCache;
	Snapshot.Resolution = Resolution;
	Snapshot.RealWorldSize = ChunkSizeWorldUnits;

	FString Path = FPaths::ProjectSavedDir() / SlotName / FString::Printf(TEXT("Chk_%d_%d.bin"), GridCoordinate.X, GridCoordinate.Y);

	(new FAutoDeleteAsyncTask<FTerraDyneAsyncSaver>(Snapshot, Path))->StartBackgroundTask();
}