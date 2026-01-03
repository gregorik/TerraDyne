#include "IO/TerraDyneSerializer.h"

// Engine Includes
#include "Misc/FileHelper.h"
#include "Misc/Compression.h"
#include "Serialization/MemoryReader.h"

// Magic Header used to verify file integrity (matches Saver)
const int32 TERRADYNE_MAGIC = 0x5444594E; // "TDYN"

bool UTerraDyneSerializer::LoadChunkFromDisk(const FString& FilePath, FTerraDyneChunkSnapshot& OutSnapshot)
{
	if (FilePath.IsEmpty()) return false;

	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *FilePath))
	{
		// File likely doesn't exist yet (not saved), which is fine.
		return false; 
	}

	return DeserializeFromBytes(FileBytes, OutSnapshot);
}

bool UTerraDyneSerializer::DeserializeFromBytes(const TArray<uint8>& Bytes, FTerraDyneChunkSnapshot& OutSnapshot)
{
	if (Bytes.Num() < 8) return false; // Too small for header

	FMemoryReader Loader(Bytes, true); // True = persistent (doesn't matter here since we copy)

	// 1. Check Header
	int32 Magic;
	Loader << Magic;
	if (Magic != TERRADYNE_MAGIC)
	{
		UE_LOG(LogTemp, Warning, TEXT("TerraDyneSerializer: Invalid File Header"));
		return false;
	}

	// 2. Read Meta
	int32 UncompressedSize = 0;
	Loader << UncompressedSize;

	// 3. Read Compressed Payload
	TArray<uint8> CompressedBuffer;
	Loader << CompressedBuffer;

	// safety check
	if (UncompressedSize <= 0 || CompressedBuffer.Num() == 0) return false;

	// 4. Decompress
	TArray<uint8> UncompressedBytes;
	UncompressedBytes.SetNum(UncompressedSize);

	bool bUncompressSuccess = FCompression::UncompressMemory(
		NAME_Zlib,
		UncompressedBytes.GetData(),
		UncompressedSize,
		CompressedBuffer.GetData(),
		CompressedBuffer.Num()
	);

	if (!bUncompressSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("TerraDyneSerializer: Decompression Failed"));
		return false;
	}

	// 5. Serialize Object Data
	FMemoryReader Ar(UncompressedBytes, true);
	return ReadSnapshotFromArchive(Ar, OutSnapshot);
}

bool UTerraDyneSerializer::ReadSnapshotFromArchive(FArchive& Ar, FTerraDyneChunkSnapshot& OutSnapshot)
{
	// Must match the write order in TerraDyneAsyncSaver.cpp
	
	int32 Version = 0;
	Ar << Version;

	if (Version == 1)
	{
		Ar << OutSnapshot.GridCoordinate;
		Ar << OutSnapshot.Resolution;
		Ar << OutSnapshot.RealWorldSize;
		Ar << OutSnapshot.HeightData; // TArray handles count+data automatically
		Ar << OutSnapshot.WeightData;
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("TerraDyneSerializer: Unknown Data Version %d"), Version);
	return false;
}