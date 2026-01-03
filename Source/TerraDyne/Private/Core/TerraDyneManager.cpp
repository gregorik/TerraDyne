#include "Core/TerraDyneManager.h"
#include "Core/TerraDyneSubsystem.h"
#include "World/TerraDyneChunk.h"

// Engine Includes
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h" 
#include "Async/ParallelFor.h"

ATerraDyneManager::ATerraDyneManager()
{
	PrimaryActorTick.bCanEverTick = false;
	GlobalChunkSize = 0.f; // Will auto-detect or can be set in Details
	ChunkClass = ATerraDyneChunk::StaticClass();
	bAutoImportAtRuntime = true;
}

void ATerraDyneManager::BeginPlay()
{
	Super::BeginPlay();

	// 1. Register with Subsystem
	if (UWorld* World = GetWorld())
	{
		if (UTerraDyneSubsystem* Subsystem = World->GetSubsystem<UTerraDyneSubsystem>())
		{
			Subsystem->RegisterManager(this);
		}
	}

	// 2. Auto-Import Logic
	if (bAutoImportAtRuntime)
	{
		RebuildChunkMap();
		if (ActiveChunkMap.Num() == 0)
		{
			// Try property first, then auto-detect
			ALandscapeProxy* Source = TargetLandscapeSource;
			if (!Source)
			{
				AActor* Found = UGameplayStatics::GetActorOfClass(GetWorld(), ALandscapeProxy::StaticClass());
				Source = Cast<ALandscapeProxy>(Found);
			}

			if (Source)
			{
				ImportInternal(Source, true);
			}
			else
			{
				SpawnDefaultSandboxChunk();
			}
		}
	}
	else
	{
		RebuildChunkMap();
	}
}

void ATerraDyneManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UTerraDyneSubsystem* Subsystem = World->GetSubsystem<UTerraDyneSubsystem>())
		{
			Subsystem->UnregisterManager(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

// --- REFLECTION HELPER ---
static UTexture2D* GetPrivateWeightmap(ULandscapeComponent* Comp)
{
	if (!Comp) return nullptr;
	FArrayProperty* Prop = CastField<FArrayProperty>(Comp->GetClass()->FindPropertyByName(FName("WeightmapTextures")));
	if (Prop)
	{
		void* Addr = Prop->ContainerPtrToValuePtr<void>(Comp);
		FScriptArrayHelper Helper(Prop, Addr);
		if (Helper.Num() > 0)
		{
			FObjectProperty* Inner = CastField<FObjectProperty>(Prop->Inner);
			return Cast<UTexture2D>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(0)));
		}
	}
	return nullptr;
}

// --- BUTTON HANDLER ---
void ATerraDyneManager::ManualImport()
{
	if (TargetLandscapeSource)
	{
		UE_LOG(LogTemp, Log, TEXT("TerraDyne: Starting Manual Import provided Landscape..."));
		ImportInternal(TargetLandscapeSource, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TerraDyne: No Target Landscape Source selected in Details Panel!"));
	}
}

// --- LIDAR LOGIC ---
void ATerraDyneManager::PerformLidarResample(ATerraDyneChunk* Chunk, ALandscapeProxy* Source)
{
	if (!Chunk || !Source) return;

	int32 Res = 128;
	Chunk->HeightCache.SetNumUninitialized(Res * Res);
	Chunk->Resolution = Res;
	float ChunkSize = Chunk->ChunkSizeWorldUnits;

	FTransform ChunkTransform = Chunk->GetActorTransform();
	UWorld* World = GetWorld();

	// Ensure we can hit the source
	Source->SetActorEnableCollision(true);

	// Parallel sweep
	ParallelFor(Res * Res, [&](int32 Index)
		{
			int32 X = Index % Res;
			int32 Y = Index / Res;

			double U = (double)X / (double)(Res - 1);
			double V = (double)Y / (double)(Res - 1);

			FVector LocalPos(U * ChunkSize, V * ChunkSize, 0.0);
			FVector WorldPos = ChunkTransform.TransformPosition(LocalPos);

			// Trace VERY High to VERY Low
			FVector Start = FVector(WorldPos.X, WorldPos.Y, 200000.0f);
			FVector End = FVector(WorldPos.X, WorldPos.Y, -200000.0f);

			FHitResult Hit;
			FCollisionQueryParams QParams;
			QParams.bTraceComplex = true;
			QParams.AddIgnoredActor(Chunk); // Ignore the new chunk itself

			bool bHit = false;

			// 1. Try Visibility
			if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QParams))
			{
				if (Hit.GetActor() == Source) bHit = true;
			}

			// 2. Fallback: WorldStatic (if Visibility is blocked by Volumes)
			if (!bHit)
			{
				if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, QParams))
				{
					if (Hit.GetActor() == Source) bHit = true;
				}
			}

			if (bHit)
			{
				// Calculate Z relative to Chunk Actor (which is at Z=0 mostly)
				Chunk->HeightCache[Index] = Hit.ImpactPoint.Z - ChunkTransform.GetLocation().Z;
			}
			else
			{
				Chunk->HeightCache[Index] = 0.0f;
			}
		});

	// Apply results immediately
	Chunk->RebuildPhysicsMesh();
	Chunk->UpdateVisualTexture();
}

// --- IMPORT LOGIC ---
void ATerraDyneManager::ImportInternal(ALandscapeProxy* Source, bool bHideSource)
{
	if (!Source) return;

	TArray<ULandscapeComponent*> Components = Source->LandscapeComponents;
	if (Components.Num() == 0) return;

	// Use user setting, or valid auto-detect
	if (GlobalChunkSize <= 10.0f)
	{
		float CompResolution = (float)Components[0]->ComponentSizeQuads;
		float Scale = Components[0]->GetComponentTransform().GetScale3D().X;
		GlobalChunkSize = CompResolution * Scale;
	}

	UE_LOG(LogTemp, Log, TEXT("TerraDyne: Importing with Chunk Size: %f"), GlobalChunkSize);

	int32 ChunksCreated = 0;

	for (ULandscapeComponent* Comp : Components)
	{
		FIntPoint GridCoord(
			FMath::RoundToInt(Comp->GetComponentLocation().X / GlobalChunkSize),
			FMath::RoundToInt(Comp->GetComponentLocation().Y / GlobalChunkSize)
		);

		FActorSpawnParameters SpawnParams;
		SpawnParams.bDeferConstruction = true;

#if WITH_EDITOR
		SpawnParams.OverrideLevel = Comp->GetOwner()->GetLevel();
#endif

		// Fix Ambiguous Ternary Operator C2445
		UClass* ClassToSpawn = (ChunkClass.Get() != nullptr) ? ChunkClass.Get() : ATerraDyneChunk::StaticClass();

		// Flatten Z to 0.0f to avoid "Level Down" issues. 
		// We handle height pure via Lidar.
		FVector SpawnLoc = Comp->GetComponentLocation();
		SpawnLoc.Z = 0.0f;
		FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLoc, Comp->GetComponentTransform().GetScale3D());

		ATerraDyneChunk* NewChunk = GetWorld()->SpawnActor<ATerraDyneChunk>(
			ClassToSpawn,
			SpawnTransform,
			SpawnParams
		);

		if (NewChunk)
		{
			// Extract Weights using reflection
			UTexture2D* WeightTex = GetPrivateWeightmap(Comp);

			// Init defaults
			NewChunk->InitializeChunk(GridCoord, GlobalChunkSize, 128, nullptr, WeightTex);

			if (MasterMaterial) NewChunk->SetMaterial(MasterMaterial);
			if (HeightBrushMaterial) NewChunk->BrushMaterialBase = HeightBrushMaterial;
			if (WeightBrushMaterial) NewChunk->PaintMaterialBase = WeightBrushMaterial;

			NewChunk->FinishSpawning(SpawnTransform);

			// Perform the Geometry Scan
			PerformLidarResample(NewChunk, Source);

#if WITH_EDITOR
			NewChunk->SetIsSpatiallyLoaded(true);
			NewChunk->SetFolderPath(FName("TerraDyne_Chunks"));
#endif

			int64 Hash = GetChunkHash(GridCoord.X, GridCoord.Y);
			ActiveChunkMap.Add(Hash, NewChunk);
			ChunksCreated++;
		}
	}

#if WITH_EDITOR
	if (bHideSource)
	{
		Source->SetActorHiddenInGame(true);
		// Note: We leave collision ON for the source so Lidar works, 
		// user can disable it later if physics overlap is an issue.
	}
#endif

	UE_LOG(LogTemp, Log, TEXT("TerraDyne: Created %d Chunks via Lidar scan."), ChunksCreated);
}

void ATerraDyneManager::SpawnDefaultSandboxChunk()
{
	GlobalChunkSize = 10000.0f;

	FActorSpawnParameters Params;
	Params.bDeferConstruction = true;

	UClass* ClassToSpawn = (ChunkClass.Get() != nullptr) ? ChunkClass.Get() : ATerraDyneChunk::StaticClass();

	ATerraDyneChunk* Chunk = GetWorld()->SpawnActor<ATerraDyneChunk>(ClassToSpawn, FTransform::Identity, Params);

	if (Chunk)
	{
		Chunk->InitializeChunk(FIntPoint(0, 0), GlobalChunkSize, 128, nullptr, nullptr);

		if (MasterMaterial) Chunk->SetMaterial(MasterMaterial);
		if (HeightBrushMaterial) Chunk->BrushMaterialBase = HeightBrushMaterial;
		if (WeightBrushMaterial) Chunk->PaintMaterialBase = WeightBrushMaterial;

		Chunk->RebuildPhysicsMesh();
		Chunk->FinishSpawning(FTransform::Identity);

		int64 Hash = GetChunkHash(0, 0);
		ActiveChunkMap.Add(Hash, Chunk);

		UE_LOG(LogTemp, Warning, TEXT("TerraDyne Automation: Sandbox Chunk Spawned."));
	}
}

void ATerraDyneManager::ApplyGlobalBrush(FVector WorldLocation, float Radius, float Strength, bool bIsHole, int32 PaintLayer)
{
	if (GlobalChunkSize <= 0.0f) { RebuildChunkMap(); if (GlobalChunkSize <= 0) return; }

	FBox2D BrushBounds(
		FVector2D(WorldLocation.X - Radius, WorldLocation.Y - Radius),
		FVector2D(WorldLocation.X + Radius, WorldLocation.Y + Radius)
	);

	int32 MinX = FMath::FloorToInt(BrushBounds.Min.X / GlobalChunkSize);
	int32 MinY = FMath::FloorToInt(BrushBounds.Min.Y / GlobalChunkSize);
	int32 MaxX = FMath::FloorToInt(BrushBounds.Max.X / GlobalChunkSize);
	int32 MaxY = FMath::FloorToInt(BrushBounds.Max.Y / GlobalChunkSize);

	for (int32 X = MinX; X <= MaxX; X++)
	{
		for (int32 Y = MinY; Y <= MaxY; Y++)
		{
			int64 Hash = GetChunkHash(X, Y);
			if (ATerraDyneChunk** FoundChunk = ActiveChunkMap.Find(Hash))
			{
				if (*FoundChunk)
				{
					FVector LocalPos = WorldLocation - (*FoundChunk)->GetActorLocation();
					(*FoundChunk)->ApplyLocalIdempotentEdit(LocalPos, Radius, Strength, bIsHole, PaintLayer);
				}
			}
		}
	}
}

ATerraDyneChunk* ATerraDyneManager::GetChunkAtLocation(FVector WorldLocation)
{
	if (GlobalChunkSize <= 0) return nullptr;

	int32 X = FMath::FloorToInt(WorldLocation.X / GlobalChunkSize);
	int32 Y = FMath::FloorToInt(WorldLocation.Y / GlobalChunkSize);

	if (ATerraDyneChunk** Chunk = ActiveChunkMap.Find(GetChunkHash(X, Y)))
	{
		return *Chunk;
	}
	return nullptr;
}

void ATerraDyneManager::RebuildChunkMap()
{
	ActiveChunkMap.Reset();
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATerraDyneChunk::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		ATerraDyneChunk* Chunk = Cast<ATerraDyneChunk>(Actor);
		if (Chunk)
		{
			if (GlobalChunkSize <= 0) GlobalChunkSize = Chunk->ChunkSizeWorldUnits;

			int64 Hash = GetChunkHash(Chunk->GridCoordinate.X, Chunk->GridCoordinate.Y);
			ActiveChunkMap.Add(Hash, Chunk);
		}
	}
}

int64 ATerraDyneManager::GetChunkHash(int32 X, int32 Y) const
{
	return ((int64)X << 32) | (uint32)Y;
}