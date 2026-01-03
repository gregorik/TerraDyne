#include "World/TerraDyneProjectile.h"
#include "Core/TerraDyneManager.h"
#include "Core/TerraDyneSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h" // Required for FObjectFinder
#include "DrawDebugHelpers.h" // Required for DrawDebugSphere

ATerraDyneProjectile::ATerraDyneProjectile()
{
	// 1. Setup Collision (The Ball)
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);

	// Load Engine Basic Sphere
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereObj(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereObj.Succeeded()) MeshComp->SetStaticMesh(SphereObj.Object);

	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetNotifyRigidBodyCollision(true);
	MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));

	// CRITICAL FIX: Bind the Hit Event! 
	// Without this, OnHit never fires and the holes won't appear.
	MeshComp->OnComponentHit.AddDynamic(this, &ATerraDyneProjectile::OnHit);

	MeshComp->SetWorldScale3D(FVector(2.0f));

	// 2. Setup Movement
	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MoveComp"));
	MovementComp->InitialSpeed = 3000.f;
	MovementComp->MaxSpeed = 5000.f;
	MovementComp->bShouldBounce = true;
	MovementComp->Bounciness = 0.3f;
}

void ATerraDyneProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only interact with the world on heavy impacts
	if (NormalImpulse.Size() < 500.0f) return;

	if (UWorld* World = GetWorld())
	{
		if (UTerraDyneSubsystem* Subsystem = World->GetSubsystem<UTerraDyneSubsystem>())
		{
			if (ATerraDyneManager* Manager = Subsystem->GetTerrainManager())
			{
				// 1. Dig the Hole (Physics Change)
				// Radius, Depth (-400), IsHole (false)
				Manager->ApplyGlobalBrush(Hit.Location, CraterRadius, CraterDepth, false);

				// 2. Paint the Magma (Visual Change)
				// Layer 1 corresponds to the Green Channel (Magma)
				Manager->ApplyGlobalBrush(Hit.Location, CraterRadius * 1.2f, 1.0f, false, 1 /* Layer 1 */);

				// 3. Visual Feedback
				//DrawDebugSphere(World, Hit.Location, CraterRadius, 16, FColor::Red, false, 2.0f);
			}
		}
	}

	// Shrink slightly on impact to eventually disappear so we don't clog physics
	SetActorScale3D(GetActorScale3D() * 0.9f);
	if (GetActorScale3D().X < 0.2f) Destroy();
}