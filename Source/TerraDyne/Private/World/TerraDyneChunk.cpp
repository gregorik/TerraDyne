#include "World/TerraDyneChunk.h"
#include "IO/TerraDyneAsyncSaver.h"

// Engine Includes
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshSpatialFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DynamicMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Async/Async.h"
#include "Rendering/Texture2DResource.h"
#include "TextureResource.h"
#include "Engine/Canvas.h" 

#include "Core/TerraDyneSubsystem.h"
#include "Grass/TerraDyneGrassSystem.h"
#include "Core/TerraDyneManager.h"

#define GRID_INDEX(X, Y) ((Y) * Resolution + (X))

ATerraDyneChunk::ATerraDyneChunk()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsSpatiallyLoaded = false;

	// 1. PHYSICS MESH 
	PhysicsMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("PhysicsMesh"));
	SetRootComponent(PhysicsMesh);

	// Collision Config
	PhysicsMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PhysicsMesh->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
	PhysicsMesh->SetGenerateOverlapEvents(true);
	PhysicsMesh->BodyInstance.bUseCCD = true;

	// Ensure it blocks the editing cursor (WorldStatic)
	PhysicsMesh->SetCollisionObjectType(ECC_WorldStatic);
	PhysicsMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Visible for debugging/verification
	PhysicsMesh->SetVisibility(true);
	PhysicsMesh->SetHiddenInGame(false);

	// 2. VISUAL MESH
	VisualMesh = CreateDefaultSubobject<UVirtualHeightfieldMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetMobility(EComponentMobility::Movable);
	VisualMesh->SetBoundsScale(100.0f); // Prevent Culling

	// Allow query for cursor (Backup)
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	VisualMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GridCoordinate = FIntPoint(0, 0);
	ChunkSizeWorldUnits = 10000.0f;
	Resolution = 128;
	bPhysicsIsDirty = false;
	CollisionUpdateDelay = 0.05f;
	ZScale = 256.0f;
}

UTextureRenderTarget2D* ATerraDyneChunk::CreateInternalRT(int32 Res, ETextureRenderTargetFormat Format, FLinearColor ClearColor)
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	RT->RenderTargetFormat = Format;
	RT->InitAutoFormat(Res, Res);
	RT->ClearColor = ClearColor;
	RT->UpdateResourceImmediate(true);
	return RT;
}

void ATerraDyneChunk::BeginPlay()
{
	Super::BeginPlay();

	if (ATerraDyneManager* Manager = Cast<ATerraDyneManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATerraDyneManager::StaticClass())))
	{
		if (!BrushMaterialBase && Manager->HeightBrushMaterial) BrushMaterialBase = Manager->HeightBrushMaterial;
		if (!PaintMaterialBase && Manager->WeightBrushMaterial) PaintMaterialBase = Manager->WeightBrushMaterial;
	}

	if (HeightCache.Num() == 0 && !LinkedTileData)
	{
		InitializeChunk(GridCoordinate, ChunkSizeWorldUnits, 128, nullptr);
		RebuildPhysicsMesh();
		UpdateVisualTexture();
	}
	else if (LinkedTileData)
	{
		InitializeFromAsset(LinkedTileData);
	}
}

void ATerraDyneChunk::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_CollisionUpdate);
	Super::EndPlay(EndPlayReason);
}

void ATerraDyneChunk::InitializeFromAsset(UTerraDyneTileData* TileData) {}
void ATerraDyneChunk::SaveAsync(FString SlotName) {}

// --- HELPER: FORCE CLEAR WEIGHT MAP ---
// Uses Canvas to write pure zeros, fixing the "Magma Overrun" noise issue.
void ClearWeightMap(UTextureRenderTarget2D* RT)
{
	if (!RT) return;
	UCanvas* Canvas; FVector2D Size; FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(RT, RT, Canvas, Size, Context);
	if (Canvas) {
		Canvas->K2_DrawBox(FVector2D(0, 0), Size, 0.0f, FLinearColor(0, 0, 0, 0));
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(RT, Context);
}

void ATerraDyneChunk::InitializeChunk(FIntPoint Coord, float Size, int32 InRes, UTexture2D* SourceHeight, UTexture2D* SourceWeight)
{
	GridCoordinate = Coord;
	ChunkSizeWorldUnits = Size;
	Resolution = InRes;

	// 1. Init Cache (Perlin)
	HeightCache.SetNumUninitialized(Resolution * Resolution);
	const float NoiseFreq = 0.05f;
	FVector Offset(Coord.X * Resolution, Coord.Y * Resolution, 0);

	for (int32 Y = 0; Y < Resolution; Y++) {
		for (int32 X = 0; X < Resolution; X++) {
			float SampleX = (X + Offset.X) * NoiseFreq;
			float SampleY = (Y + Offset.Y) * NoiseFreq;
			float Z = 0.5f + (FMath::Sin(SampleX) * FMath::Cos(SampleY) * 0.2f);
			HeightCache[GRID_INDEX(X, Y)] = Z;
		}
	}

	// 2. Init RTs
	HeightRT = CreateInternalRT(Resolution, RTF_R16f, FLinearColor::Black);
	WeightRT = CreateInternalRT(Resolution, RTF_RGBA8, FLinearColor(0, 0, 0, 0));
	ClearWeightMap(WeightRT); // FORCE CLEAR

	// 3. Build Interactions
	RebuildPhysicsMesh();
	SyncPhysicsGeometry();
	PhysicsMesh->UpdateCollision(false);
	UpdateVisualTexture();
}

void ATerraDyneChunk::RebuildPhysicsMesh()
{
	if (!PhysicsMesh) return;
	PhysicsMesh->GetDynamicMesh()->Reset();

	FGeometryScriptPrimitiveOptions Options;
	// FIX UVs: Map UVs 0..1 across the grid. 
	// Without this, textures stretch infinitely (green magma everywhere).
	Options.UVMode = EGeometryScriptPrimitiveUVMode::Uniform;

	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(
		PhysicsMesh->GetDynamicMesh(), Options, FTransform::Identity,
		ChunkSizeWorldUnits, ChunkSizeWorldUnits, Resolution / 2, Resolution / 2
	);

	PhysicsMesh->UpdateCollision(false);
}

void ATerraDyneChunk::ApplyLocalIdempotentEdit(FVector RelativePos, float Radius, float Strength, bool bIsHole, int32 PaintLayer)
{
	if (HeightCache.Num() == 0) return;

	if (PaintLayer >= 0) {
		ApplyPaintBrush(GetActorLocation() + RelativePos, Radius, 1.0f, PaintLayer);
		return;
	}

	float NormStrength = Strength / ZScale;
	if (FMath::Abs(NormStrength) < 0.0001f) return;

	// ADDED REL2D DECLARATION
	FVector2D Rel2D(RelativePos.X, RelativePos.Y);

	float HalfSize = ChunkSizeWorldUnits * 0.5f;
	float gridX = ((RelativePos.X + HalfSize) / ChunkSizeWorldUnits) * (Resolution - 1);
	float gridY = ((RelativePos.Y + HalfSize) / ChunkSizeWorldUnits) * (Resolution - 1);
	float radGrid = (Radius / ChunkSizeWorldUnits) * (Resolution - 1);

	int32 minX = FMath::Clamp(FMath::FloorToInt(gridX - radGrid), 0, Resolution - 1);
	int32 maxX = FMath::Clamp(FMath::CeilToInt(gridX + radGrid), 0, Resolution - 1);
	int32 minY = FMath::Clamp(FMath::FloorToInt(gridY - radGrid), 0, Resolution - 1);
	int32 maxY = FMath::Clamp(FMath::CeilToInt(gridY + radGrid), 0, Resolution - 1);

	bool bModified = false;

	if (!bIsHole) {
		for (int32 y = minY; y <= maxY; y++) {
			for (int32 x = minX; x <= maxX; x++) {
				float U = (float)x / (float)(Resolution - 1);
				float V = (float)y / (float)(Resolution - 1);
				float PX = (U * ChunkSizeWorldUnits) - HalfSize;
				float PY = (V * ChunkSizeWorldUnits) - HalfSize;

				if (FVector2D::Distance(Rel2D, FVector2D(PX, PY)) <= Radius) {
					float Dist = FVector2D::Distance(Rel2D, FVector2D(PX, PY));
					float Alpha = FMath::Square(1.0f - (Dist / Radius));
					int32 idx = GRID_INDEX(x, y);
					HeightCache[idx] += NormStrength * Alpha;
					bModified = true;
				}
			}
		}
	}

	if (!bModified) return;

	if (!TerrainMID && BrushMaterialBase) {
		TerrainMID = UMaterialInstanceDynamic::Create(BrushMaterialBase, this);
	}

	if (TerrainMID && HeightRT) {
		FVector2D BrushUV((RelativePos.X + HalfSize) / ChunkSizeWorldUnits, (RelativePos.Y + HalfSize) / ChunkSizeWorldUnits);
		TerrainMID->SetVectorParameterValue(TEXT("BrushPos"), FLinearColor(BrushUV.X, BrushUV.Y, 0, 0));
		TerrainMID->SetScalarParameterValue(TEXT("Radius"), Radius / ChunkSizeWorldUnits);
		TerrainMID->SetScalarParameterValue(TEXT("Strength"), NormStrength);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, HeightRT, TerrainMID);
	}

	bPhysicsIsDirty = true;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_CollisionUpdate, this, &ATerraDyneChunk::PerformDeferredCollisionUpdate, CollisionUpdateDelay, false);
}

void ATerraDyneChunk::ApplyPaintBrush(FVector WorldPos, float Radius, float Strength, int32 LayerChannel)
{
	if (!PaintMaterialBase || !WeightRT) return;
	UMaterialInstanceDynamic* PaintMID = UMaterialInstanceDynamic::Create(PaintMaterialBase, this);
	FVector LocalPos = GetActorTransform().InverseTransformPosition(WorldPos);
	float HalfSize = ChunkSizeWorldUnits * 0.5f;
	FVector2D UV((LocalPos.X + HalfSize) / ChunkSizeWorldUnits, (LocalPos.Y + HalfSize) / ChunkSizeWorldUnits);
	PaintMID->SetVectorParameterValue(TEXT("BrushPos"), FLinearColor(UV.X, UV.Y, 0, 0));
	PaintMID->SetScalarParameterValue(TEXT("Radius"), Radius / ChunkSizeWorldUnits);
	PaintMID->SetScalarParameterValue(TEXT("Strength"), 1.0f);
	FLinearColor Mask(LayerChannel == 0 ? 1 : 0, LayerChannel == 1 ? 1 : 0, LayerChannel == 2 ? 1 : 0, LayerChannel == 3 ? 1 : 0);
	PaintMID->SetVectorParameterValue(TEXT("ChannelMask"), Mask);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, WeightRT, PaintMID);
}

void ATerraDyneChunk::PerformDeferredCollisionUpdate()
{
	if (!bPhysicsIsDirty) return;
	SyncPhysicsGeometry();
	PhysicsMesh->UpdateCollision(true);
	bPhysicsIsDirty = false;
}

void ATerraDyneChunk::SyncPhysicsGeometry()
{
	PhysicsMesh->GetDynamicMesh()->EditMesh([&](FDynamicMesh3& Mesh)
		{
			for (int32 vid : Mesh.VertexIndicesItr())
			{
				FVector3d Pos = Mesh.GetVertex(vid);
				float HalfSize = ChunkSizeWorldUnits * 0.5f;
				int32 X = FMath::Clamp(FMath::RoundToInt(((Pos.X + HalfSize) / ChunkSizeWorldUnits) * (Resolution - 1)), 0, Resolution - 1);
				int32 Y = FMath::Clamp(FMath::RoundToInt(((Pos.Y + HalfSize) / ChunkSizeWorldUnits) * (Resolution - 1)), 0, Resolution - 1);
				float NewZ = HeightCache[GRID_INDEX(X, Y)] * ZScale;
				Mesh.SetVertex(vid, FVector3d(Pos.X, Pos.Y, NewZ));
			}
		}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::VertexPositions);
}

void ATerraDyneChunk::UpdateVisualTexture()
{
	if (!HeightRT || HeightCache.Num() == 0) return;
	TArray<FFloat16Color> RawData; RawData.SetNum(HeightCache.Num());
	for (int32 i = 0; i < HeightCache.Num(); i++) RawData[i] = FFloat16Color(FLinearColor(HeightCache[i], 0, 0, 1));

	UTexture2D* TempTex = UTexture2D::CreateTransient(Resolution, Resolution, PF_FloatRGBA);
	if (TempTex) {
		TempTex->CompressionSettings = TC_HDR; TempTex->SRGB = 0; TempTex->AddToRoot();
		FTexture2DMipMap& Mip = TempTex->GetPlatformData()->Mips[0];
		void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, RawData.GetData(), RawData.Num() * sizeof(FFloat16Color));
		Mip.BulkData.Unlock(); TempTex->UpdateResource();

		UCanvas* Canvas; FVector2D Size; FDrawToRenderTargetContext Context;
		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, HeightRT, Canvas, Size, Context);
		if (Canvas) Canvas->K2_DrawTexture(TempTex, FVector2D(0, 0), Size, FVector2D(0, 0), FVector2D(1, 1), FLinearColor::White, EBlendMode::BLEND_Opaque);
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
		TempTex->RemoveFromRoot();
	}
}

void ATerraDyneChunk::SetMaterial(UMaterialInterface* InMaterial)
{
	if (!VisualMesh || !InMaterial) return;
	PhysicsMesh->SetMaterial(0, InMaterial);
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(InMaterial, this);
	MID->SetTextureParameterValue(TEXT("HeightMap"), HeightRT);
	MID->SetTextureParameterValue(TEXT("WeightMap"), WeightRT);
	MID->SetScalarParameterValue(TEXT("ZScale"), ZScale);
	if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(VisualMesh)) PrimComp->SetMaterial(0, MID);
}