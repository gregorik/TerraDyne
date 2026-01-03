#include "Baking/TerraDyneBaker.h"
#include "World/TerraDyneTileData.h"

// Engine Includes
#include "LandscapeProxy.h"
#include "LandscapeComponent.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

TArray<UTerraDyneTileData*> UTerraDyneBaker::BakeLandscapeToAssets(ALandscapeProxy* SourceLandscape, FString DestinationPath)
{
    TArray<UTerraDyneTileData*> CreatedAssets;

    if (!SourceLandscape)
    {
        UE_LOG(LogTemp, Warning, TEXT("TerraDyneBaker: Invalid Source Landscape."));
        return CreatedAssets;
    }

    UE_LOG(LogTemp, Log, TEXT("TerraDyneBaker: Starting massive bake for %s..."), *SourceLandscape->GetName());

    // Iterate over every landscape component (Tile)
    for (ULandscapeComponent* Comp : SourceLandscape->LandscapeComponents)
    {
        if (Comp)
        {
            // Auto-generate name based on grid coordinates (e.g. "TD_Tile_0_0")
            // SectionBaseX/Y gives the absolute coordinates in quads
            FString DefaultName = FString::Printf(TEXT("TD_Tile_%d_%d"), Comp->SectionBaseX, Comp->SectionBaseY);

            UTerraDyneTileData* NewAsset = BakeComponent(Comp, DestinationPath, DefaultName);
            
            if (NewAsset)
            {
                CreatedAssets.Add(NewAsset);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TerraDyneBaker: Bake Complete. Created %d Assets."), CreatedAssets.Num());
    return CreatedAssets;
}

UTerraDyneTileData* UTerraDyneBaker::BakeComponent(ULandscapeComponent* Component, FString DestinationPath, FString AssetNameOverride)
{
    if (!Component) return nullptr;

    // 1. Determine Asset Name and Path
    FString AssetName = AssetNameOverride;
    if (AssetName.IsEmpty())
    {
        AssetName = FString::Printf(TEXT("TD_Tile_%d_%d"), Component->SectionBaseX, Component->SectionBaseY);
    }
    
    // Ensure path ends with /
    if (!DestinationPath.EndsWith(TEXT("/")))
    {
        DestinationPath += TEXT("/");
    }

    FString PackageName = DestinationPath + AssetName;

    // 2. Create the Asset Package
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    // Create the UObject
    UTerraDyneTileData* NewData = NewObject<UTerraDyneTileData>(Package, *AssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);

    // 3. Extract Metadata
    float QuadCount = Component->GetComponentSizeQuads(); // e.g., 63
    float ScaleX = Component->GetComponentTransform().GetScale3D().X;
    float ScaleZ = Component->GetComponentTransform().GetScale3D().Z;

    NewData->RealWorldSize = QuadCount * ScaleX;
    NewData->BakedZScale = ScaleZ * 128.0f; // *128 is a specific engine constant for max landscape height range usually
    // Store grid coordinates for debugging
    NewData->GridCoordinate = FIntPoint(Component->SectionBaseX, Component->SectionBaseY);
    NewData->SourceComponentName = Component->GetName();

    // 4. Extract Heavy Data
    int32 HeightRes = 0;
    if (ExtractHeightmapData(Component, NewData->InitialHeightMap, HeightRes))
    {
        NewData->Resolution = HeightRes;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TerraDyneBaker: Failed to extract Heightmap for %s"), *AssetName);
    }

    int32 WeightRes = 0;
    if (ExtractWeightmapData(Component, NewData->InitialWeightMap, WeightRes))
    {
        // Warn if resolutions mismatch?
    }

    // 5. Save and Register
    NewData->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewData);

    // Optional: Auto-Save the asset to disk immediately
    /*
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewData, *FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()), SaveArgs);
    */

    return NewData;
}

bool UTerraDyneBaker::ExtractHeightmapData(ULandscapeComponent* Comp, TArray<uint16>& OutData, int32& OutRes)
{
    UTexture2D* HeightTex = Comp->GetHeightmap();
    if (!HeightTex) return false;

    // In Editor, we can access the Source Data or Platform Data.
    // PlatformData gives us what is effectively compiled for the platform.
    
    // 1. Prepare to read Mips
    // Note: This requires the texture to be CPU accessible or Source info available.
    // For standard Baking, Source is preferred, but here we show PlatformData access 
    // which works on loaded textures.
    FTexturePlatformData* PlatformData = HeightTex->GetPlatformData();
    if (!PlatformData || PlatformData->Mips.Num() == 0) return false;

    int32 Width = PlatformData->Mips[0].SizeX;
    int32 Height = PlatformData->Mips[0].SizeY;
    OutRes = Width;

    OutData.Reset();
    OutData.SetNumUninitialized(Width * Height);

    // 2. Lock Buffer
    // Landscape Heightmaps are typically BGRA8 or R8G8.
    // Standard UE Landscape separates 16-bit height into R and G channels.
    const void* RawData = PlatformData->Mips[0].BulkData.Lock(LOCK_READ_ONLY);
    
    if (RawData)
    {
        // Assuming BGRA format (Typical)
        const FColor* Pixels = static_cast<const FColor*>(RawData);

        for (int32 i = 0; i < OutData.Num(); i++)
        {
            const FColor& P = Pixels[i];
            
            // Decode UE Landscape Height
            // Usually: ((R << 8) | G) - 32768 for World Z.
            // But we want raw local 0-65535 storage.
            // Note: Channel order depends on platform. Usually R=High, G=Low.
            
            uint16 HeightValue = (P.R << 8) | P.G;
            OutData[i] = HeightValue;
        }
    }
    else
    {
        PlatformData->Mips[0].BulkData.Unlock();
        return false;
    }

    PlatformData->Mips[0].BulkData.Unlock();
    return true;
}

bool UTerraDyneBaker::ExtractWeightmapData(ULandscapeComponent* Comp, TArray<FColor>& OutData, int32& OutRes)
{
    // Landscape layers are distributed across multiple textures if >4 layers.
    // For this baker, we grab index 0 (the primary texture).
    UTexture2D* WeightTex = Comp->GetWeightmapTexture(0);
    if (!WeightTex) return false;

    FTexturePlatformData* PlatformData = WeightTex->GetPlatformData();
    if (!PlatformData || PlatformData->Mips.Num() == 0) return false;

    int32 Width = PlatformData->Mips[0].SizeX;
    int32 Height = PlatformData->Mips[0].SizeY;
    OutRes = Width;

    OutData.Reset();
    OutData.SetNumUninitialized(Width * Height);

    const void* RawData = PlatformData->Mips[0].BulkData.Lock(LOCK_READ_ONLY);

    if (RawData)
    {
        // Raw copy of weights
        const FColor* Pixels = static_cast<const FColor*>(RawData);
        FMemory::Memcpy(OutData.GetData(), Pixels, OutData.Num() * sizeof(FColor));
    }
    else
    {
        PlatformData->Mips[0].BulkData.Unlock();
        return false;
    }

    PlatformData->Mips[0].BulkData.Unlock();
    return true;
}