#include "World/TerraDyneTileData.h"

UTerraDyneTileData::UTerraDyneTileData()
{
	Resolution = 128;
	RealWorldSize = 10000.0f; // Default 100m
	BakedZScale = 100.0f;
	GridCoordinate = FIntPoint(0, 0);
}

int32 UTerraDyneTileData::GetMemoryFootprint() const
{
	int32 HeightBytes = InitialHeightMap.Num() * sizeof(uint16);
	int32 WeightBytes = InitialWeightMap.Num() * sizeof(FColor);
	return HeightBytes + WeightBytes;
}