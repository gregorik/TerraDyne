#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TerraDyneBaker.generated.h"

// Forward Declarations
class ULandscapeComponent;
class ALandscapeProxy;
class UTerraDyneTileData;

/**
 * UTerraDyneBaker
 * 
 * Editor-only utility library responsible for converting Stock Unreal Landscapes 
 * into TerraDyne's optimized binary asset format (UTerraDyneTileData).
 * 
 * This processes high-res textures (height/weight) and quantizes/compresses 
 * them for efficient runtime streaming.
 */
UCLASS()
class TERRADYNEEDITOR_API UTerraDyneBaker : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Main Entry Point.
	 * Bakes an entire Landscape Actor into a set of UTerraDyneTileData assets.
	 * 
	 * @param SourceLandscape   The actor to convert.
	 * @param DestinationPath   Package path to save assets (e.g. "/Game/Map/TerraDyne").
	 * @return                  List of created data assets.
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Baking")
	static TArray<UTerraDyneTileData*> BakeLandscapeToAssets(ALandscapeProxy* SourceLandscape, FString DestinationPath);

	/**
	 * Bakes a single Landscape Component into a Data Asset.
	 * Useful for partial updates or selective baking.
	 * 
	 * @param Component         The specific component to process.
	 * @param DestinationPath   Directory to save the asset.
	 * @param AssetNameOverride Optional name. IF Key is empty, generates name based on Grid Coordinates.
	 * @return                  The created and saved Data Asset.
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Baking")
	static UTerraDyneTileData* BakeComponent(ULandscapeComponent* Component, FString DestinationPath, FString AssetNameOverride = TEXT(""));

private:
	/**
	 * Internal helper to lock and read Landscape Source Texture data.
	 * Extracts 16-bit height values from the specific RG channel encoding used by UE Landscape.
	 */
	static bool ExtractHeightmapData(ULandscapeComponent* Comp, TArray<uint16>& OutData, int32& OutRes);

	/**
	 * Internal helper to read Weightmap textures.
	 * Extracts channel usage for the first 4 layers (RGBA).
	 */
	static bool ExtractWeightmapData(ULandscapeComponent* Comp, TArray<FColor>& OutData, int32& OutRes);
};