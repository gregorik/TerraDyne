#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "TerraDynePaver.generated.h"

UCLASS()
class TERRADYNE_API ATerraDynePaver : public AActor
{
	GENERATED_BODY()

public:
	ATerraDynePaver();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> CutterMesh; // Visual spinner

	// Settings
	float MoveSpeed = 1000.0f;
	float DigRadius = 1200.0f;
	float DigDepthPerTick = -30.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	// Internal logic to stay glued to ground
	void AdjustHeightToGround();

	// Spawns visual debris
	void SpewRubble();
};