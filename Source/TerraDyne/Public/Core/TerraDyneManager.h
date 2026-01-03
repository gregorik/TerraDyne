#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerraDyneManager.generated.h"

// Forward Declarations
class ATerraDyneChunk;
class ALandscapeProxy;
class UMaterialInterface;

UCLASS(Blueprintable)
class TERRADYNE_API ATerraDyneManager : public AActor
{
	GENERATED_BODY()

public:
	ATerraDyneManager();

	//--- CONFIGURATION ---

	// Assign your Landscape here manually if auto-detection fails.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Import Settings")
	TObjectPtr<ALandscapeProxy> TargetLandscapeSource;

	// Use this to override the chunk size. Default 0 detects from landscape.
	// Try 10000 (100m) or 50000 (500m) if import creates too many/too few chunks.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Import Settings")
	float GlobalChunkSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Config")
	TSubclassOf<ATerraDyneChunk> ChunkClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Config")
	TObjectPtr<UMaterialInterface> MasterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Tools")
	TObjectPtr<UMaterialInterface> HeightBrushMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Tools")
	TObjectPtr<UMaterialInterface> WeightBrushMaterial;

	//--- BUTTONS ---

	// The "Magic Button". Select TargetLandscapeSource above, then click this.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TerraDyne|Tools")
	void ManualImport();

	//--- RUNTIME ---

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Interaction")
	void ApplyGlobalBrush(FVector WorldLocation, float Radius, float Strength, bool bIsHole, int32 PaintLayer = -1);

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Query")
	ATerraDyneChunk* GetChunkAtLocation(FVector WorldLocation);

	// Regenerates the internal lookup map. Call if you spawn chunks manually.
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|System")
	void RebuildChunkMap();

	// Automation: If TRUE, tries to find a landscape at BeginPlay and import it.
	UPROPERTY(EditAnywhere, Category = "TerraDyne|Automation")
	bool bAutoImportAtRuntime = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TMap<int64, ATerraDyneChunk*> ActiveChunkMap;

	int64 GetChunkHash(int32 X, int32 Y) const;
	void SpawnDefaultSandboxChunk();
	void PerformLidarResample(ATerraDyneChunk* Chunk, ALandscapeProxy* Source);
	void ImportInternal(ALandscapeProxy* Source, bool bHideSource);
};