#include "World/TerraDynePaver.h"
#include "Core/TerraDyneManager.h"
#include "Core/TerraDyneSubsystem.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "World/TerraDyneProjectile.h" // We reuse the sphere as rubble

ATerraDynePaver::ATerraDynePaver()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. The Main Chassis
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(BodyMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeObj(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeObj.Succeeded()) BodyMesh->SetStaticMesh(CubeObj.Object);

	BodyMesh->SetWorldScale3D(FVector(8.0f, 15.0f, 4.0f)); // Wide platform
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Kinematic visuals only

	// 2. The Cutter (Visual)
	CutterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cutter"));
	CutterMesh->SetupAttachment(BodyMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylObj(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylObj.Succeeded()) CutterMesh->SetStaticMesh(CylObj.Object);

	CutterMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	CutterMesh->SetRelativeLocation(FVector(80.0f, 0.0f, -50.0f)); // Front and low
	CutterMesh->SetWorldScale3D(FVector(1.5f, 1.5f, 1.2f)); // Relative scale
}

void ATerraDynePaver::BeginPlay()
{
	Super::BeginPlay();
	// Start high to avoid clipping initial spawn
	FVector StartPos = GetActorLocation();
	StartPos.Z = 1000.0f;
	SetActorLocation(StartPos);
}

void ATerraDynePaver::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Move Forward (Kinematic)
	FVector NewLoc = GetActorLocation() + (GetActorForwardVector() * MoveSpeed * DeltaTime);
	SetActorLocation(NewLoc);

	// 2. Spin Cutter
	CutterMesh->AddLocalRotation(FRotator(0, 0, 500.0f * DeltaTime));

	// 3. Dig Logic
	if (UWorld* World = GetWorld())
	{
		if (UTerraDyneSubsystem* Subsystem = World->GetSubsystem<UTerraDyneSubsystem>())
		{
			if (ATerraDyneManager* Manager = Subsystem->GetTerrainManager())
			{
				// Dig slightly in front of the center
				FVector DigSpot = GetActorLocation() + (GetActorForwardVector() * 500.0f);
				DigSpot.Z -= 200.0f; // Dig down

				// Massive Dig
				Manager->ApplyGlobalBrush(DigSpot, DigRadius, DigDepthPerTick, false);
				// Paint Magma
				Manager->ApplyGlobalBrush(DigSpot, DigRadius * 0.9f, 1.0f, false, 1);
			}
		}
	}

	// 4. Stay grounded (Visual Raycast)
	AdjustHeightToGround();

	// 5. Spawn Physics Proof
	SpewRubble();
}

void ATerraDynePaver::AdjustHeightToGround()
{
	// Trace down to find where the "Wheels" should be
	FVector TraceStart = GetActorLocation() + FVector(0, 0, 1000);
	FVector TraceEnd = GetActorLocation() - FVector(0, 0, 5000); // Look deep into the pit

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		// Smoothly interpolate to new floor height
		FVector Cur = GetActorLocation();
		float TargetZ = Hit.ImpactPoint.Z + 150.0f; // Hover slightly above floor
		Cur.Z = FMath::FInterpTo(Cur.Z, TargetZ, GetWorld()->GetDeltaSeconds(), 2.0f);
		SetActorLocation(Cur);
	}
}

void ATerraDynePaver::SpewRubble()
{
	// Spawn 5 times a second
	if (FMath::RandRange(0, 10) > 2) return;

	// Eject from the back
	FVector SpawnLoc = GetActorLocation() - (GetActorForwardVector() * 800.0f);
	SpawnLoc.Z += 800.0f; // High enough to fall

	// Reuse the Projectile class as "Rubble" since it has physics enabled
	if (UWorld* World = GetWorld())
	{
		FRotator RandomRot = FRotator(FMath::RandRange(0, 360), FMath::RandRange(0, 360), 0);
		World->SpawnActor<ATerraDyneProjectile>(ATerraDyneProjectile::StaticClass(), SpawnLoc, RandomRot);
	}
}