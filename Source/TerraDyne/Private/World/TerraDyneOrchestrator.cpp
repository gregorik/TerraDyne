#include "World/TerraDyneOrchestrator.h"
#include "World/TerraDyneProjectile.h"
#include "Core/TerraDyneManager.h"
#include "Core/TerraDyneSubsystem.h"

ATerraDyneOrchestrator::ATerraDyneOrchestrator()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATerraDyneOrchestrator::BeginPlay()
{
	Super::BeginPlay();
	
	// auto-spawn the manager if missing?
	// For this demo, we assume BP_TerraDyneManager is in the level.
}

void ATerraDyneOrchestrator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Demo Logic: Every 0.1 seconds, spawn a meteor in a spiral pattern
	TimeElapsed += DeltaTime;
	if (TimeElapsed > 0.05f) 
	{
		TimeElapsed = 0;
		
		// 1. Math for Spiral
		SpiralAngle += 0.2f;
		Radius += 10.0f;
		if (Radius > 8000.0f) { Radius = 0; SpiralAngle = 0; } // Reset loop

		FVector Center = GetActorLocation();
		float X = Center.X + Radius * FMath::Cos(SpiralAngle);
		float Y = Center.Y + Radius * FMath::Sin(SpiralAngle);
		FVector SpawnPos(X, Y, Center.Z + 4000.0f); // High up

		// 2. Aim at ground
		FRotator Rotation = FRotator(-90.0f, 0.0f, 0.0f); // Point down

		// 3. Spawn
		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<ATerraDyneProjectile>(ATerraDyneProjectile::StaticClass(), SpawnPos, Rotation);
		}
	}
}