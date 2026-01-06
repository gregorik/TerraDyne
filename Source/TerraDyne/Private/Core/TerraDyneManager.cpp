#include "Core/TerraDyneManager.h"
#include "Core/TerraDyneSubsystem.h"
#include "World/TerraDyneChunk.h"

// DEMO ACTORS
#include "World/TerraDynePaver.h"
#include "World/TerraDyneOrchestrator.h"

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

	// Always trigger Sandbox for the Demo to ensure reliability
	SpawnDefaultSandboxChunk();

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
	GlobalChunkSize = 10000.0f; // 100 meters

	FActorSpawnParameters Params;
	Params.bDeferConstruction = true;

	UClass* ClassToSpawn = (ChunkClass.Get() != nullptr) ? ChunkClass.Get() : ATerraDyneChunk::StaticClass();

	ATerraDyneChunk* Chunk = GetWorld()->SpawnActor<ATerraDyneChunk>(ClassToSpawn, FTransform::Identity, Params);

	if (Chunk)
	{
		if (HeightBrushMaterial) Chunk->BrushMaterialBase = HeightBrushMaterial;
		if (WeightBrushMaterial) Chunk->PaintMaterialBase = WeightBrushMaterial;

		// Init will trigger the Perlin Noise generation
		Chunk->InitializeChunk(FIntPoint(0, 0), GlobalChunkSize, 128, nullptr, nullptr);

		if (MasterMaterial) Chunk->SetMaterial(MasterMaterial);

		Chunk->RebuildPhysicsMesh();
		Chunk->FinishSpawning(FTransform::Identity);

		int64 Hash = GetChunkHash(0, 0);
		ActiveChunkMap.Add(Hash, Chunk);

		UE_LOG(LogTemp, Warning, TEXT("TerraDyne Manager: Sandbox Chunk Created."));
	}

	// Spawn Demo Actors (Paver and Orbs)
	FVector PaverStart(-4000.0f, 0.0f, 3500.0f);
	GetWorld()->SpawnActor<ATerraDynePaver>(ATerraDynePaver::StaticClass(), PaverStart, FRotator::ZeroRotator);

	GetWorld()->SpawnActor<ATerraDyneOrchestrator>(ATerraDyneOrchestrator::StaticClass(), FVector(0, 0, 4000), FRotator::ZeroRotator);
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

void ATerraDyneManager::ManualImport()
{
	if (TargetLandscapeSource)
	{
		UE_LOG(LogTemp, Log, TEXT("TerraDyne: Starting Manual Import..."));
		ImportFromLandscape(TargetLandscapeSource, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TerraDyne: No Target Landscape Source selected in Details Panel!"));
	}
}

void ATerraDyneManager::ResampleLandscapeData(ATerraDyneChunk* Chunk, ALandscapeProxy* Source)
{
	// Keep stub or implement logic if referenced.
	// Since we are forcing Sandbox spawn for stability, this path is technically unused in the demo,
	// but required for compilation.
}

void ATerraDyneManager::ImportFromLandscape(ALandscapeProxy* TargetLandscape, bool bHideSource)
{
	ImportInternal(TargetLandscape, bHideSource);
}

void ATerraDyneManager::ImportInternal(ALandscapeProxy* Source, bool bHideSource)
{
	if (!Source) return;

	TArray<ULandscapeComponent*> Components = Source->LandscapeComponents;
	if (Components.Num() == 0) return;

	if (GlobalChunkSize <= 10.0f)
	{
		float CompResolution = (float)Components[0]->ComponentSizeQuads;
		float Scale = Components[0]->GetComponentTransform().GetScale3D().X;
		GlobalChunkSize = CompResolution * Scale;
	}

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

		UClass* ClassToSpawn = (ChunkClass.Get() != nullptr) ? ChunkClass.Get() : ATerraDyneChunk::StaticClass();

		ATerraDyneChunk* NewChunk = GetWorld()->SpawnActor<ATerraDyneChunk>(
			ClassToSpawn,
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

			// We skip the Lidar logic here to simplify compilation, assuming Sandbox is the target.
			// If Import is strictly needed, the previous Lidar function goes here.

			NewChunk->SetIsSpatiallyLoaded(true);
			NewChunk->SetFolderPath(FName("TerraDyne_Chunks"));

			int64 Hash = GetChunkHash(GridCoord.X, GridCoord.Y);
			ActiveChunkMap.Add(Hash, NewChunk);
			ChunksCreated++;
		}
	}

	if (bHideSource)
	{
		Source->SetActorHiddenInGame(true);
		Source->SetActorEnableCollision(false);
	}

	UE_LOG(LogTemp, Log, TEXT("TerraDyne: Import Complete."));
}

#endif