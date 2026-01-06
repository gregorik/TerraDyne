#include "World/TerraDyneProjectile.h"
#include "Core/TerraDyneManager.h"
#include "Core/TerraDyneSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

ATerraDyneProjectile::ATerraDyneProjectile()
{
	// 1. Setup Mesh & Physics
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereObj(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereObj.Succeeded()) MeshComp->SetStaticMesh(SphereObj.Object);

	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetNotifyRigidBodyCollision(true);
	MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));

	// --- FIX: ENABLE CCD (Anti-Tunneling) ---
	// This prevents high-speed objects from passing through thin dynamic meshes.
	MeshComp->BodyInstance.bUseCCD = true;

	MeshComp->OnComponentHit.AddDynamic(this, &ATerraDyneProjectile::OnHit);

	MeshComp->SetWorldScale3D(FVector(2.0f));

	// 2. Setup Movement (Sync with Physics)
	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MoveComp"));
	MovementComp->InitialSpeed = 3000.f;
	MovementComp->MaxSpeed = 5000.f;
	MovementComp->bShouldBounce = true;
	MovementComp->Bounciness = 0.3f;

	// --- FIX: SWEEP AND UPDATE ---
	MovementComp->bSweepCollision = true; // Check for collisions during movement update
	MovementComp->SetUpdatedComponent(MeshComp);
}

void ATerraDyneProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (NormalImpulse.Size() < 100.0f) return; // Lower threshold to ensure we catch bouncing

	if (UWorld* World = GetWorld())
	{
		if (UTerraDyneSubsystem* Subsystem = World->GetSubsystem<UTerraDyneSubsystem>())
		{
			if (ATerraDyneManager* Manager = Subsystem->GetTerrainManager())
			{
				// Deform
				Manager->ApplyGlobalBrush(Hit.Location, CraterRadius, CraterDepth, false);
				// Paint (Visuals)
				Manager->ApplyGlobalBrush(Hit.Location, CraterRadius * 1.2f, 1.0f, false, 1);
			}
		}
	}

	SetActorScale3D(GetActorScale3D() * 0.9f);
	if (GetActorScale3D().X < 0.2f) Destroy();
}