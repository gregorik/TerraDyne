#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

/**
 * FTerraDyneChunkSnapshot
 * 
 * A stable copy of the Chunk's state captured on the Game Thread.
 * This ensures validity while the background thread processes the data.
 */
struct FTerraDyneChunkSnapshot
{
	FIntPoint GridCoordinate;
	
	// Physics Data (Source of truth for shape)
	TArray<float> HeightData;
	
	// Layer Data (R/G/B/A weights)
	// Captured from the RT via ReadPixels (GameThread only!) before dispatch.
	TArray<FColor> WeightData;
	
	// Metadata
	int32 Resolution;
	float RealWorldSize;
	
	// Empty check
	bool IsValid() const 
	{ 
		return HeightData.Num() > 0; 
	}
};

/**
 * FTerraDyneAsyncSaver
 * 
 * The worker class for FAutoDeleteAsyncTask.
 * Performs ZLib compression and file writing.
 * 
 * Usage:
 * (new FAutoDeleteAsyncTask<FTerraDyneAsyncSaver>(Snapshot, FilePath))->StartBackgroundTask();
 */
class TERRADYNE_API FTerraDyneAsyncSaver : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FTerraDyneAsyncSaver>;

public:
	FTerraDyneAsyncSaver(const FTerraDyneChunkSnapshot& InSnapshot, const FString& InFilePath)
		: Snapshot(InSnapshot)
		, FilePath(InFilePath)
	{
	}

	/**
	 * The Heavy Lifting.
	 * 1. Serializes Snapshot to binary buffer.
	 * 2. Compresses buffer using Zlib.
	 * 3. Writes File to Disk.
	 */
	void DoWork();

	/** Thread Stat ID for profiling */
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FTerraDyneAsyncSaver, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	FTerraDyneChunkSnapshot Snapshot;
	FString FilePath;
};

/**
 * UTerraDyneIOSubsystem
 * 
 * Optional helper statics for Path Management.
 */
class TERRADYNE_API FTerraDyneIOPaths
{
public:
	static FString GetSaveSlotPath(FString SlotName);
	static FString GetChunkFilename(FIntPoint Coord);
};