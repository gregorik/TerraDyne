#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IO/TerraDyneAsyncSaver.h" // For FTerraDyneChunkSnapshot definition
#include "TerraDyneSerializer.generated.h"

/**
 * UTerraDyneSerializer
 * 
 * Static Helper Library for TerraDyne I/O operations.
 * Implements the "Read" counterpart to FTerraDyneAsyncSaver's "Write".
 * 
 * Responsibilities:
 * 1. Validating File Headers (Magic Numbers).
 * 2. ZLib Decompression.
 * 3. Binary Deserialization into Snapshot structs.
 */
UCLASS()
class TERRADYNE_API UTerraDyneSerializer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Synchronously loads a chunk file from disk and populates the Snapshot struct.
	 * 
	 * @param FilePath      Absolute path to the .bin file.
	 * @param OutSnapshot   The struct to populate with Height/Weight data.
	 * @return              True if load and decompression were successful.
	 */
	static bool LoadChunkFromDisk(const FString& FilePath, FTerraDyneChunkSnapshot& OutSnapshot);

	/**
	 * Deserializes raw binary data (e.g. from network or memory cache) into a Snapshot.
	 * Handles the decompression step internally.
	 * 
	 * @param Bytes         The raw file bytes (Compressed).
	 * @param OutSnapshot   The struct to populate.
	 * @return              True if successful.
	 */
	static bool DeserializeFromBytes(const TArray<uint8>& Bytes, FTerraDyneChunkSnapshot& OutSnapshot);

private:
	// Internal helper to read our specific versioned binary format
	static bool ReadSnapshotFromArchive(FArchive& Ar, FTerraDyneChunkSnapshot& OutSnapshot);
};