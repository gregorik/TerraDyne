#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeometryScript/GeometryScriptSelectionTypes.h" // For FGeometryScriptMeshSelection
#include "TerraDyneMeshUtils.generated.h"

// Forward Declarations
class UDynamicMeshComponent;
class UDynamicMesh;

/**
 * UTerraDyneMeshUtils
 * 
 * A stateless utility library for procedural geometry operations specific to the TerraDyne system.
 * Wraps GeometryScript functionality to provide high-level "Terrain" operations.
 */
UCLASS()
class TERRADYNE_API UTerraDyneMeshUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Generates a planar grid mesh optimized for TerraDyne's collision needs.
	 * 
	 * @param TargetMesh       The dynamic mesh to write to.
	 * @param Size             World size of the grid (e.g., 51200.0).
	 * @param Resolution       Vertex resolution (e.g., 128 for 128x128 grid).
	 * @param MaterialID       Default material ID for the ground (usually 0).
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Geometry")
	static void GenerateTerrainGrid(
		UDynamicMesh* TargetMesh, 
		float Size, 
		int32 Resolution, 
		int32 MaterialID = 0
	);

	/**
	 * modifications the Material ID of triangles within a radius to create a logical hole.
	 * The Chunk's material must support masking on the designated HoleMaterialID.
	 * 
	 * @param TargetMesh       The dynamic mesh to modify.
	 * @param LocalCenter      Center of the brush in Chunk Local Space.
	 * @param Radius           Radius of the hole.
	 * @param HoleMaterialID   The ID to assign (usually 1).
	 * @param bRefill          If true, reverts the ID back to Ground (Material 0).
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Geometry")
	static void UpdateHoleAtLocation(
		UDynamicMesh* TargetMesh, 
		FVector LocalCenter, 
		float Radius, 
		int32 HoleMaterialID = 1,
		bool bRefill = false
	);

	/**
	 * Recalculates normals for the mesh. 
	 * While Physics/Collision doesn't need visuals, ensuring correct vertex normals
	 * can be helpful if debug rendering is enabled or for specific raycast logic.
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraDyne|Geometry")
	static void RecomputeNormals(UDynamicMesh* TargetMesh);
};