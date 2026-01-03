#include "IO/TerraDyneAsyncSaver.h"

// Engine Includes
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Compression.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryWriter.h"
#include "HAL/PlatformFilemanager.h"

void FTerraDyneAsyncSaver::DoWork()
{
	if (!Snapshot.IsValid())
	{
		return;
	}

	// 1. Serialize Raw Data to Memory
	TArray<uint8> UncompressedBuffer;
	FMemoryWriter Writer(UncompressedBuffer);

	// --- FILE FORMAT VERSION 1 ---
	int32 Version = 1;
	Writer << Version;

	// Identity
	Writer << Snapshot.GridCoordinate;
	Writer << Snapshot.Resolution;
	Writer << Snapshot.RealWorldSize;

	// Payloads
	Writer << Snapshot.HeightData;
	Writer << Snapshot.WeightData;

	// 2. Compress Data
	TArray<uint8> CompressedBuffer;
	// FIX: Must be non-const for serialization operators, even if writing
	int32 UncompressedSize = UncompressedBuffer.Num();

	int32 CompressedSize = FCompression::CompressMemoryBound(NAME_Zlib, UncompressedSize);
	CompressedBuffer.AddUninitialized(CompressedSize);

	bool bCompressionSuccess = FCompression::CompressMemory(
		NAME_Zlib,
		CompressedBuffer.GetData(),
		CompressedSize,
		UncompressedBuffer.GetData(),
		UncompressedSize
	);

	if (bCompressionSuccess)
	{
		CompressedBuffer.SetNum(CompressedSize);

		// 3. Final Package 
		FBufferArchive FinalArchive;

		int32 MagicHeader = 0x5444594E; // "TDYN"
		FinalArchive << MagicHeader;
		FinalArchive << UncompressedSize; // This line crashed before because UncompressedSize was const
		FinalArchive << CompressedBuffer;

		// 4. Write to Disk
		FString Folder = FPaths::GetPath(FilePath);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*Folder))
		{
			PlatformFile.CreateDirectoryTree(*Folder);
		}

		FFileHelper::SaveArrayToFile(FinalArchive, *FilePath);
	}
}

//--- Path Helpers ---

FString FTerraDyneIOPaths::GetSaveSlotPath(FString SlotName)
{
	return FPaths::ProjectSavedDir() / TEXT("TerraDyne") / SlotName;
}

FString FTerraDyneIOPaths::GetChunkFilename(FIntPoint Coord)
{
	return FString::Printf(TEXT("TD_Chunk_%d_%d.bin"), Coord.X, Coord.Y);
}