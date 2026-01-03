#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/DynamicMeshComponent.h"
#include "VirtualHeightfieldMeshComponent.h"
#include "World/TerraDyneTileData.h"
#include "Engine/TextureRenderTarget2D.h" // Critical for ETextureRenderTargetFormat
#include "TerraDyneChunk.generated.h"

// Forward Declarations
class UMaterialInstanceDynamic;

/**
 * ATerraDyneChunk
 *
 * Represents a single streamable tile of the landscape.
 * Updates: Includes public material slots for tools to avoid hardcoded path failures.
 */
UCLASS(Config = Game)
class TERRADYNE_API ATerraDyneChunk : public AActor
{
	GENERATED_BODY()

public:
	ATerraDyneChunk();

	friend class ATerraDyneManager; // Allow Manager to write directly to HeightCache during Import

	//--- Components ---//

	// The geometric base. Handles collision and holes.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne|Components")
	TObjectPtr<UDynamicMeshComponent> PhysicsMesh;

	// The visual representation. Uses huge displacement map for details.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne|Components")
	TObjectPtr<UVirtualHeightfieldMeshComponent> VisualMesh;

	//--- Resources (Transient Runtime) ---//

	// The active Heightmap on the GPU. Driven by HeightCache.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "TerraDyne|Runtime")
	TObjectPtr<UTextureRenderTarget2D> HeightRT;

	// The active Weightmap (Layers) on the GPU.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "TerraDyne|Runtime")
	TObjectPtr<UTextureRenderTarget2D> WeightRT;

	//--- Tool Injection (Critical Fixes) ---//

	// Injected by Manager to ensure we can draw heights without LoadObject() failures
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BrushMaterialBase;

	// Injected by Manager to ensure we can draw layers
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PaintMaterialBase;

	//--- Data & Config ---//

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne|Identity")
	FIntPoint GridCoordinate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne|Identity")
	float ChunkSizeWorldUnits;

	// Phase 4: The baked static data source. 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TerraDyne|Identity")
	TObjectPtr<UTerraDyneTileData> LinkedTileData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Config")
	float ZScale = 100.0f;

	UPROPERTY(EditAnywhere, Category = "TerraDyne|Performance")
	float CollisionUpdateDelay = 0.1f;

	//--- Public API ---//

	/** Initializes the chunk from raw parameters. */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Init")
	void InitializeChunk(FIntPoint Coord, float Size, int32 Resolution, UTexture2D* SourceHeight, UTexture2D* SourceWeight = nullptr);

	/** Initializes from baked asset. */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Init")
	void InitializeFromAsset(UTerraDyneTileData* TileData);

	/**
	 * Forces regeneration of the low-poly physics grid.
	 * Critical for "Sandbox Mode" to ensure the orbs have something to hit.
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Physics")
	void RebuildPhysicsMesh();

	/** Modifies the terrain geometry (Dig/Raise). */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Edit")
	void ApplyLocalIdempotentEdit(FVector RelativePos, float Radius, float Strength, bool bIsHole, int32 PaintLayer = -1);

	/** Paints material layers. */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Edit")
	void ApplyPaintBrush(FVector WorldPos, float Radius, float Strength, int32 LayerChannel);

	/** Material helper function. */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Visuals")
	void SetMaterial(UMaterialInterface* InMaterial);

	/** Async Saving. */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|IO")
	void SaveAsync(FString SlotName);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	//--- Internal Data ---//

	// CPU-side Single Source of Truth for Height.
	TArray<float> HeightCache;
	int32 Resolution;

	// Timer for Anti-Stutter system
	FTimerHandle TimerHandle_CollisionUpdate;
	bool bPhysicsIsDirty;

	// Cached Material Instance to drive RT parameters
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TerrainMID;

	//--- Private Helpers ---//

	void SyncPhysicsGeometry();
	void UpdateVisualTexture();

	UFUNCTION()
	void PerformDeferredCollisionUpdate();

	UTextureRenderTarget2D* CreateInternalRT(int32 Res, ETextureRenderTargetFormat Format, FLinearColor ClearColor);
};