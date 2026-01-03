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

	//--- Automation ---//
	UPROPERTY(EditAnywhere, Category = "TerraDyne|Automation")
	bool bAutoImportAtRuntime = true;

	//--- Config ---//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Config")
	TSubclassOf<ATerraDyneChunk> ChunkClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Config")
	TObjectPtr<UMaterialInterface> MasterMaterial;

	//--- Tools Injection ---//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Tools")
	TObjectPtr<UMaterialInterface> HeightBrushMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Tools")
	TObjectPtr<UMaterialInterface> WeightBrushMaterial;

	//--- Debug/State ---//
	UPROPERTY(VisibleAnywhere, Category = "TerraDyne|Debug")
	float GlobalChunkSize;

	//--- Runtime API ---//

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Interaction")
	void ApplyGlobalBrush(FVector WorldLocation, float Radius, float Strength, bool bIsHole, int32 PaintLayer = -1);

	// FIX: Added missing declaration here
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Query")
	ATerraDyneChunk* GetChunkAtLocation(FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|System")
	void RebuildChunkMap();

	//--- Editor/Import API ---//
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TerraDyne|Tools")
	void ImportFromLandscape(ALandscapeProxy* TargetLandscape, bool bHideSource = true);

	// Helper for resampling
	void ResampleLandscapeData(ATerraDyneChunk* Chunk, ALandscapeProxy* Source);
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TMap<int64, ATerraDyneChunk*> ActiveChunkMap;

	int64 GetChunkHash(int32 X, int32 Y) const;
	void SpawnDefaultSandboxChunk();
};