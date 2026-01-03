#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerraDyneOrchestrator.generated.h"

UCLASS()
class TERRADYNE_API ATerraDyneOrchestrator : public AActor
{
	GENERATED_BODY()
	
public:	
	ATerraDyneOrchestrator();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	float TimeElapsed;
	float SpiralAngle;
	float Radius;
};