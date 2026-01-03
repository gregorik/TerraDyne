#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/DynamicMeshComponent.h"
#include "VirtualHeightfieldMeshComponent.h"
#include "World/TerraDyneTileData.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h" // Required for UpdateVisualTexture logic

// The generated header MUST be the last include
#include "TerraDyneChunk.generated.h"

// Forward Declarations
class UMaterialInstanceDynamic;

/**
 * ATerraDyneChunk
 *
 * Represents a single streamable tile of the landscape.
 */
UCLASS(Config = Game)
class TERRADYNE_API ATerraDyneChunk : public AActor
{
	GENERATED_BODY()

	// CRITICAL: Allow Manager to write directly to HeightCache during Lidar Import
	friend class ATerraDyneManager;

public:
	ATerraDyneChunk();

	//--- Components ---//

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne|Components")
	TObjectPtr<UDynamicMeshComponent> PhysicsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne|Components")
	TObjectPtr<UVirtualHeightfieldMeshComponent> VisualMesh;

	//--- Resources (Transient Runtime) ---//

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "TerraDyne|Runtime")
	TObjectPtr<UTextureRenderTarget2D> HeightRT;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "TerraDyne|Runtime")
	TObjectPtr<UTextureRenderTarget2D> WeightRT;

	//--- Tool Injection ---//

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BrushMaterialBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PaintMaterialBase;

	//--- Data & Config ---//

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne|Identity")
	FIntPoint GridCoordinate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne|Identity")
	float ChunkSizeWorldUnits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TerraDyne|Identity")
	TObjectPtr<UTerraDyneTileData> LinkedTileData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne|Config")
	float ZScale = 100.0f;

	UPROPERTY(EditAnywhere, Category = "TerraDyne|Performance")
	float CollisionUpdateDelay = 0.1f;

	//--- Public API ---//

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Init")
	void InitializeChunk(FIntPoint Coord, float Size, int32 Resolution, UTexture2D* SourceHeight, UTexture2D* SourceWeight = nullptr);

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Init")
	void InitializeFromAsset(UTerraDyneTileData* TileData);

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Physics")
	void RebuildPhysicsMesh();

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Edit")
	void ApplyLocalIdempotentEdit(FVector RelativePos, float Radius, float Strength, bool bIsHole, int32 PaintLayer = -1);

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Edit")
	void ApplyPaintBrush(FVector WorldPos, float Radius, float Strength, int32 LayerChannel);

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Visuals")
	void SetMaterial(UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintCallable, Category = "TerraDyne|IO")
	void SaveAsync(FString SlotName);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	//--- Internal Data ---//

	TArray<float> HeightCache;
	int32 Resolution;

	FTimerHandle TimerHandle_CollisionUpdate;
	bool bPhysicsIsDirty;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TerrainMID;

	//--- Private Helpers ---//

	void SyncPhysicsGeometry();
	void UpdateVisualTexture();

	UFUNCTION()
	void PerformDeferredCollisionUpdate();

	UTextureRenderTarget2D* CreateInternalRT(int32 Res, ETextureRenderTargetFormat Format, FLinearColor ClearColor);
};