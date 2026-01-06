#include "World/TerraDyneSceneSetup.h"
#include "Core/TerraDyneManager.h"
#include "World/TerraDyneChunk.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "UObject/ConstructorHelpers.h"

ATerraDyneSceneSetup::ATerraDyneSceneSetup()
{
	PrimaryActorTick.bCanEverTick = false;

	// Try to load defaults automatically
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MasterMat(TEXT("/Game/TerraDyne/Materials/VHFM/M_TerraDyne_Master"));
	if (MasterMat.Succeeded()) DefaultLandscapeMaterial = MasterMat.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> HBrush(TEXT("/Game/TerraDyne/Materials/Tools/M_HeightBrush"));
	if (HBrush.Succeeded()) HeightTool = HBrush.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WBrush(TEXT("/Game/TerraDyne/Materials/Tools/M_WeightBrush"));
	if (WBrush.Succeeded()) WeightTool = WBrush.Object;
}

void ATerraDyneSceneSetup::InitializeWorld()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. Clean old Managers
	TArray<AActor*> OldManagers;
	UGameplayStatics::GetAllActorsOfClass(World, ATerraDyneManager::StaticClass(), OldManagers);
	for (AActor* Act : OldManagers) Act->Destroy();

	// 2. Spawn Manager at Origin
	FActorSpawnParameters Params;

	// FIX C2445: Explicitly resolve TSubclassOf to UClass* using .Get()
	UClass* ClassToSpawn = ManagerClass.Get() ? ManagerClass.Get() : ATerraDyneManager::StaticClass();

	ATerraDyneManager* Manager = World->SpawnActor<ATerraDyneManager>(
		ClassToSpawn,
		FVector::ZeroVector, FRotator::ZeroRotator, Params
	);

	if (Manager)
	{
		Manager->SetActorLabel(TEXT("TerraDyne_System_Manager"));

		// Inject Assets
		Manager->MasterMaterial = DefaultLandscapeMaterial;
		Manager->HeightBrushMaterial = HeightTool;
		Manager->WeightBrushMaterial = WeightTool;

		// Disable Auto-Import (We are making a new world)
		Manager->bAutoImportAtRuntime = false;

		// Set Grid Size (1km tiles)
		Manager->GlobalChunkSize = 100000.0f;

		// 3. Spawn a 2x2 Grid (4km squared world)
		for (int32 x = -1; x <= 0; x++)
		{
			for (int32 y = -1; y <= 0; y++)
			{
				FVector SpawnLoc(x * Manager->GlobalChunkSize, y * Manager->GlobalChunkSize, 0);

				// FIX C2445: Explicitly resolve TSubclassOf to UClass*
				UClass* ChunkClassToSpawn = Manager->ChunkClass.Get() ? Manager->ChunkClass.Get() : ATerraDyneChunk::StaticClass();

				// Spawn Chunk
				ATerraDyneChunk* Chunk = World->SpawnActor<ATerraDyneChunk>(
					ChunkClassToSpawn,
					SpawnLoc, FRotator::ZeroRotator, Params
				);

				if (Chunk)
				{
					// Init as Flat Plain
					Chunk->InitializeChunk(FIntPoint(x, y), Manager->GlobalChunkSize, 256, nullptr); // Higher Res 256
					Chunk->SetMaterial(DefaultLandscapeMaterial);
					Chunk->BrushMaterialBase = HeightTool;
					Chunk->PaintMaterialBase = WeightTool;
					Chunk->RebuildPhysicsMesh();

					// Force update visuals
					Chunk->ApplyLocalIdempotentEdit(FVector(0), 10.0f, 0.0f, false);

					Manager->RebuildChunkMap(); // Register it
				}
			}
		}
	}

	// 4. Lighting
	SpawnLighting();

	UE_LOG(LogTemp, Log, TEXT("TerraDyne: New World Initialized."));
#endif
}

void ATerraDyneSceneSetup::SpawnLighting()
{
	UWorld* World = GetWorld();
	// Only spawn if dark
	if (!UGameplayStatics::GetActorOfClass(World, ADirectionalLight::StaticClass()))
	{
		AActor* Sun = World->SpawnActor<AActor>(ADirectionalLight::StaticClass(), FVector(0, 0, 5000), FRotator(-45, 45, 0));
		if (Sun) Sun->SetActorLabel("TerraDyne_Sun");
	}
}