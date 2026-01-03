#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TerraDyneProjectile.generated.h"

UCLASS()
class TERRADYNE_API ATerraDyneProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ATerraDyneProjectile();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> MovementComp;

	UPROPERTY(EditAnywhere, Category = "Impact")
	float CraterRadius = 600.0f;

	UPROPERTY(EditAnywhere, Category = "Impact")
	float CraterDepth = -400.0f; // Negative = Dig

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};