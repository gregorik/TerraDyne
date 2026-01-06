#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerraDyneSceneSetup.generated.h"

// Forward Declarations
class ATerraDyneManager;
class UMaterialInterface;

/**
 * A helper tool for ons-boarding. 
 * Drastically simplifies setting up a new TerraDyne level.
 */
UCLASS(Blueprintable)
class TERRADYNE_API ATerraDyneSceneSetup : public AActor
{
	GENERATED_BODY()
	
public:	
	ATerraDyneSceneSetup();

	//--- Configuration Assets ---//
	UPROPERTY(EditAnywhere, Category = "TerraDyne Setup")
	TObjectPtr<UMaterialInterface> DefaultLandscapeMaterial;

	UPROPERTY(EditAnywhere, Category = "TerraDyne Setup")
	TObjectPtr<UMaterialInterface> HeightTool;

	UPROPERTY(EditAnywhere, Category = "TerraDyne Setup")
	TObjectPtr<UMaterialInterface> WeightTool;

	UPROPERTY(EditAnywhere, Category = "TerraDyne Setup")
	TSubclassOf<ATerraDyneManager> ManagerClass;

	//--- Actions ---//
	
	// The "One Button Solution"
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TerraDyne Setup")
	void InitializeWorld();

private:
	void SpawnLighting();
};