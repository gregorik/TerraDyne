#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/DynamicMeshComponent.h"
#include "VirtualHeightfieldMeshComponent.h"
#include "World/TerraDyneTileData.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h" 
#include "TerraDyneChunk.generated.h"

class UMaterialInstanceDynamic;

UCLASS(Config = Game)
class TERRADYNE_API ATerraDyneChunk : public AActor
{
	GENERATED_BODY()

	friend class ATerraDyneManager;

public:
	ATerraDyneChunk();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne")
	TObjectPtr<UDynamicMeshComponent> PhysicsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne")
	TObjectPtr<UVirtualHeightfieldMeshComponent> VisualMesh;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> HeightRT;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> WeightRT;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BrushMaterialBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PaintMaterialBase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne")
	FIntPoint GridCoordinate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraDyne")
	float ChunkSizeWorldUnits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TerraDyne")
	TObjectPtr<UTerraDyneTileData> LinkedTileData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraDyne")
	float ZScale = 256.0f;

	UPROPERTY(EditAnywhere, Category = "TerraDyne")
	float CollisionUpdateDelay = 0.05f;

	// API
	UFUNCTION(BlueprintCallable)
	void InitializeChunk(FIntPoint Coord, float Size, int32 InRes, UTexture2D* SourceHeight, UTexture2D* SourceWeight = nullptr);

	UFUNCTION(BlueprintCallable)
	void RebuildPhysicsMesh();

	UFUNCTION(BlueprintCallable)
	void ApplyLocalIdempotentEdit(FVector RelativePos, float Radius, float Strength, bool bIsHole, int32 PaintLayer = -1);

	UFUNCTION(BlueprintCallable)
	void ApplyPaintBrush(FVector WorldPos, float Radius, float Strength, int32 LayerChannel);

	UFUNCTION(BlueprintCallable)
	void SetMaterial(UMaterialInterface* InMaterial);

	void InitializeFromAsset(UTerraDyneTileData* TileData);
	void SaveAsync(FString SlotName);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TArray<float> HeightCache;
	int32 Resolution;
	FTimerHandle TimerHandle_CollisionUpdate;
	bool bPhysicsIsDirty;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TerrainMID;

	void SyncPhysicsGeometry();
	void UpdateVisualTexture(); // Pushes HeightCache to GPU
	UFUNCTION()
	void PerformDeferredCollisionUpdate();
	UTextureRenderTarget2D* CreateInternalRT(int32 Res, ETextureRenderTargetFormat Format, FLinearColor ClearColor);
};