#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TerraDyneToolWidget.generated.h"

UENUM(BlueprintType)
enum class ETerraDyneToolMode : uint8
{
	SculptRaise,
	SculptLower,
	Paint
};

/**
 * Native logic for the Runtime Level Editor UI.
 */
UCLASS(Abstract)
class TERRADYNE_API UTerraDyneToolWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//--- State Variables (accesible by BP and C++) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools")
	ETerraDyneToolMode CurrentTool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools")
	float BrushRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools")
	float BrushStrength = 50.0f; // For Sculpt

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools")
	int32 ActiveLayerIndex = 0;

	//--- Commands ---

	UFUNCTION(BlueprintCallable, Category = "Tools")
	void SaveWorld();

	UFUNCTION(BlueprintCallable, Category = "Tools")
	void ResetTerrain(); // Flattens chunk
};