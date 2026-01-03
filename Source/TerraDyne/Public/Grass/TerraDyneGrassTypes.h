#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "TerraDyneGrassTypes.generated.h"

USTRUCT(BlueprintType)
struct TERRADYNE_API FTerraDyneGrassVariety
{
	GENERATED_BODY()

	FTerraDyneGrassVariety()
		: GrassMesh(nullptr)
		, Density(100.0f)
		, WeightLayerIndex(0) // Moved up to match declaration order
		, MinWeight(0.2f)
		, ScaleRange(0.8f, 1.2f)
		, bAlignToSurface(true)
		, MaxSlopeAngle(45.0f)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
	TObjectPtr<UStaticMesh> GrassMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.1", ClampMax = "1000.0"))
	float Density;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 WeightLayerIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transforms")
	FVector2D ScaleRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transforms")
	bool bAlignToSurface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxSlopeAngle;
};

UCLASS(BlueprintType)
class TERRADYNE_API UTerraDyneGrassProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grass")
	TArray<FTerraDyneGrassVariety> Varieties;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

struct FTerraDyneGrassInstance
{
	FTransform Transform;
	int32 VarietyIndex;

	bool IsValid() const
	{
		return !Transform.GetLocation().ContainsNaN();
	}
};