#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/DynamicMeshComponent.h" 
#include "TerraDyneCollision.generated.h"

/**
 * UTerraDyneCollisionLib
 * 
 * Helper library for TerraDyne's geometry operations.
 * Centralizes the math used to map "HeightCache Arrays" onto "DynamicMesh Vertices".
 */
UCLASS()
class TERRADYNE_API UTerraDyneCollisionLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * The core "Sync" function.
	 * Projects the vertices of the DynamicMeshComponent onto the provided HeightData array.
	 * 
	 * Thread Safety: This functions uses EditMesh() internally, which locks the mesh.
	 * It is safe to call from the Game Thread.
	 * 
	 * @param TargetMeshComp   The component to modify.
	 * @param HeightData       Source of truth (CPU Cache) for Z heights.
	 * @param Resolution       The resolution of the HeightData grid (e.g., 128).
	 * @param ChunkSize        The physical width of the chunk in World Units.
	 * @param ZScale           Vertical scaling factor.
	 */
	static void ApplyHeightDataToMesh(
		UDynamicMeshComponent* TargetMeshComp, 
		const TArray<float>& HeightData, 
		int32 Resolution, 
		float ChunkSize,
		float ZScale = 1.0f
	);

	/**
	 * Configures a DynamicMeshComponent for optimal interaction with the Chaos Physics solver
	 * in a Landscape context.
	 * 
	 * Optimization:
	 * - Sets Collision to ComplexAsSimple (No convex hulls).
	 * - Disables Graphics features (Shadows, Decals) since this mesh is invisible.
	 * - Sets Navigation Step processing.
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Physics")
	static void ConfigureForTerrainPhysics(UDynamicMeshComponent* TargetComponent);

	/**
	 * Helper to check if a specific world location hits a hole.
	 * Uses the proprietary TerraDyne bitmask or geometry attributes to determine passability.
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Physics")
	static bool IsLocationTraceable(UDynamicMeshComponent* Mesh, FVector LocalLocation);
};