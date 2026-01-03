#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TerraDyneTileData.generated.h"

/**
 * UTerraDyneTileData
 * 
 * A static data asset containing baked terrain information for a specific chunk.
 * 
 * Workflow:
 * 1. Editor: Use UTerraDyneBaker to convert Landscape Components into these assets.
 * 2. Disk: Stored as compressed .uasset files.
 * 3. Runtime: Streamed in by ATerraDyneChunk::InitializeFromAsset().
 */
UCLASS(BlueprintType)
class TERRADYNE_API UTerraDyneTileData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UTerraDyneTileData();

	//--- Data Payloads ---//

	/** 
	 * Quantized Height Data (16-bit Unsigned).
	 * Range: 0 (MinHeight) to 65535 (MaxHeight).
	 * This array is usually (Resolution * Resolution) in length.
	 * Using uint16 reduces memory footprint by 50% compared to float.
	 */
	 // Blueprints cannot read uint16 directly. We rely on C++ accessors if needed.
	UPROPERTY(VisibleAnywhere, Category = "Baked Data")
	TArray<uint16> InitialHeightMap;

	/**
	 * Layer Weight Data (RGBA).
	 * R = Layer 1 opacity
	 * G = Layer 2 opacity
	 * B = Layer 3 opacity
	 * A = Layer 4 opacity
	 * For >4 layers, you would need a second array or a custom struct.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Baked Data")
	TArray<FColor> InitialWeightMap;

	//--- Metadata ---//

	/** The vertex resolution of the tile (e.g., 128, 256). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dimensions")
	int32 Resolution;

	/** The physical size of this tile in Unreal Units (cm). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dimensions")
	float RealWorldSize;

	//--- Scaling Info ---//

	/** 
	 * The Z-Scale multiplier used during baking.
	 * Used to de-quantize the uint16 data back to World Z floats.
	 * Standard calculation: Height(float) = (Data / 65535.0f) * ZScale * 512.0f;
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scaling")
	float BakedZScale;

	/** 
	 * World Coordinate identifier (optional, mostly for debugging). 
	 * Helps identify which grid slot this asset belongs to.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Identity")
	FIntPoint GridCoordinate;

#if WITH_EDITORONLY_DATA
	/** Source Landscape Component name (for re-baking traceability). */
	UPROPERTY(VisibleAnywhere, Category = "Source Info")
	FString SourceComponentName;
#endif

public:
	/**
	 * Helper to get the number of bytes this asset consumes in memory.
	 */
	UFUNCTION(BlueprintPure, Category = "TerraDyne")
	int32 GetMemoryFootprint() const;
};