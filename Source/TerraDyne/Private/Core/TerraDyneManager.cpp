#include "Core/TerraDyneManager.h" // MUST BE FIRST
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
	GlobalChunkSize = 0.f;
	ChunkClass = ATerraDyneChunk::StaticClass();
	bAutoImportAtRuntime = true;
}

void ATerraDyneManager::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UTerraDyneSubsystem* Subsystem = World->GetSubsystem<UTerraDyneSubsystem>())
		{
			Subsystem->RegisterManager(this);
		}
	}

	// Auto-Import Check
	if (bAutoImportAtRuntime)
	{
		AActor* LandscapeActor = UGameplayStatics::GetActorOfClass(GetWorld(), ALandscapeProxy::StaticClass());

		RebuildChunkMap();

		if (ActiveChunkMap.Num() == 0)
		{
			if (ALandscapeProxy* LandProxy = Cast<ALandscapeProxy>(LandscapeActor))
			{
#if WITH_EDITOR
				ImportFromLandscape(LandProxy, true);
#endif
			}
			else
			{
				SpawnDefaultSandboxChunk();
			}
		}
	}

	RebuildChunkMap();
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

void ATerraDyneManager::SpawnDefaultSandboxChunk()
{
	GlobalChunkSize = 10000.0f;

	FActorSpawnParameters Params;
	Params.bDeferConstruction = true;
	ATerraDyneChunk* Chunk = GetWorld()->SpawnActor<ATerraDyneChunk>(ChunkClass, FTransform::Identity, Params);

	if (Chunk)
	{
		Chunk->InitializeChunk(FIntPoint(0, 0), GlobalChunkSize, 128, nullptr);

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
	if (GlobalChunkSize <= 0.0f)
	{
		RebuildChunkMap();
		if (GlobalChunkSize <= 0) return;
	}

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

#if WITH_EDITOR

// Helper for private weightmap access
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

// Resampling Function
void ATerraDyneManager::ResampleLandscapeData(ATerraDyneChunk* Chunk, ALandscapeProxy* Source)
{
	if (!Chunk || !Source) return;

	FTransform ChunkTransform = Chunk->GetActorTransform();
	int32 Res = 128; // Standard Res

	// Ensure cache is ready
	Chunk->HeightCache.SetNumUninitialized(Res * Res);
	float ChunkSize = Chunk->ChunkSizeWorldUnits;

	ParallelFor(Res * Res, [&](int32 Index)
		{
			int32 X = Index % Res;
			int32 Y = Index / Res;

			double U = (double)X / (double)(Res - 1);
			double V = (double)Y / (double)(Res - 1);

			FVector LocalPos(U * ChunkSize, V * ChunkSize, 0.0);
			FVector WorldPos = ChunkTransform.TransformPosition(LocalPos);

			TOptional<float> HeightVal = Source->GetHeightAtLocation(WorldPos);

			if (HeightVal.IsSet())
			{
				Chunk->HeightCache[Index] = HeightVal.GetValue() - ChunkTransform.GetLocation().Z;
			}
			else
			{
				Chunk->HeightCache[Index] = 0.0f;
			}
		});

	// Finalize
	Chunk->RebuildPhysicsMesh();
	// Force a visual flush
	Chunk->ApplyLocalIdempotentEdit(FVector(0), 10.0f, 0.0f, false);
}

void ATerraDyneManager::ImportFromLandscape(ALandscapeProxy* SourceLandscape, bool bHideSource)
{
	if (!SourceLandscape) return;

	TArray<ULandscapeComponent*> Components = SourceLandscape->LandscapeComponents;
	if (Components.Num() == 0) return;

	float CompResolution = (float)Components[0]->ComponentSizeQuads;
	float Scale = Components[0]->GetComponentTransform().GetScale3D().X;
	GlobalChunkSize = CompResolution * Scale;

	int32 ChunksCreated = 0;

	for (ULandscapeComponent* Comp : Components)
	{
		FIntPoint GridCoord(
			FMath::RoundToInt(Comp->GetComponentLocation().X / GlobalChunkSize),
			FMath::RoundToInt(Comp->GetComponentLocation().Y / GlobalChunkSize)
		);

		FActorSpawnParameters SpawnParams;
		SpawnParams.bDeferConstruction = true;
		SpawnParams.OverrideLevel = Comp->GetOwner()->GetLevel();

		ATerraDyneChunk* NewChunk = GetWorld()->SpawnActor<ATerraDyneChunk>(
			ChunkClass,
			Comp->GetComponentTransform(),
			SpawnParams
		);

		if (NewChunk)
		{
			UTexture2D* HeightTex = Comp->GetHeightmap();
			UTexture2D* WeightTex = GetPrivateWeightmap(Comp);

			NewChunk->InitializeChunk(GridCoord, GlobalChunkSize, 128, HeightTex, WeightTex);

			if (MasterMaterial) NewChunk->SetMaterial(MasterMaterial);
			if (HeightBrushMaterial) NewChunk->BrushMaterialBase = HeightBrushMaterial;
			if (WeightBrushMaterial) NewChunk->PaintMaterialBase = WeightBrushMaterial;

			NewChunk->FinishSpawning(Comp->GetComponentTransform());

			// THE FIX: Resample physical data
			ResampleLandscapeData(NewChunk, SourceLandscape);

			NewChunk->SetIsSpatiallyLoaded(true);
			NewChunk->SetFolderPath(FName("TerraDyne_Chunks"));

			int64 Hash = GetChunkHash(GridCoord.X, GridCoord.Y);
			ActiveChunkMap.Add(Hash, NewChunk);
			ChunksCreated++;
		}
	}

	if (bHideSource)
	{
		SourceLandscape->SetActorHiddenInGame(true);
		SourceLandscape->SetActorEnableCollision(false);
	}

	UE_LOG(LogTemp, Log, TEXT("TerraDyne: Import Complete. Resampled %d Chunks."), ChunksCreated);
}

#endif