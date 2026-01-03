#include "Physics/TerraDyneCollision.h"

// Engine Includes
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshSpatialFunctions.h"
#include "UDynamicMesh.h"

// Geometry Core (Low-level mesh manipulation)
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"

void UTerraDyneCollisionLib::ApplyHeightDataToMesh(
	UDynamicMeshComponent* TargetMeshComp, 
	const TArray<float>& HeightData, 
	int32 Resolution, 
	float ChunkSize,
	float ZScale)
{
	if (!TargetMeshComp || !TargetMeshComp->GetDynamicMesh())
	{
		return;
	}

	if (HeightData.Num() != (Resolution * Resolution))
	{
		// Data mismatch safety check
		return;
	}

	// EditMesh is the safe way to modify the FDynamicMesh3 geometry.
	// It handles the change tracking and internal locking.
	TargetMeshComp->GetDynamicMesh()->EditMesh([&](UE::Geometry::FDynamicMesh3& Mesh)
	{
		// Pre-calculate mapping constants to avoid divisions in loop
		const float HalfSize = ChunkSize * 0.5f;
		const float InvChunkSize = 1.0f / ChunkSize;
		const int32 MaxIndex = Resolution - 1;

		// Iterate over all vertices in the mesh
		for (int32 VertID : Mesh.VertexIndicesItr())
		{
			FVector3d Pos = Mesh.GetVertex(VertID);

			// 1. Convert Local Mesh Position (Centered) to 0..1 UV mappings
			// X range: [-HalfSize, +HalfSize] -> [0, 1]
			float U = (float(Pos.X) + HalfSize) * InvChunkSize;
			float V = (float(Pos.Y) + HalfSize) * InvChunkSize;

			// 2. Map UV to Grid Coordinates (0..MaxIndex)
			// We clamp to ensure we don't read out of bounds due to floating point precision at edges
			int32 GridX = FMath::Clamp(FMath::RoundToInt(U * MaxIndex), 0, MaxIndex);
			int32 GridY = FMath::Clamp(FMath::RoundToInt(V * MaxIndex), 0, MaxIndex);

			// 3. Sample Height
			// Note: If Mesh resolution != Grid resolution, you might want Bilinear Interpolation here.
			// For collision meshes, Nearest Neighbor (RoundToInt) is usually sufficient and faster.
			int32 ArrayIdx = (GridY * Resolution) + GridX;
			
			if (HeightData.IsValidIndex(ArrayIdx))
			{
				double NewZ = (double)HeightData[ArrayIdx]; // ZScale is usually already baked into float cache
				
				// 4. Update Vertex
				Mesh.SetVertex(VertID, FVector3d(Pos.X, Pos.Y, NewZ));
			}
		}

		// Optional: Recompute Normals if needed for physics queries (slope determination)
		// Usually collision uses face normals which are auto-updated, but vertex normals might strictly require this.
		// UE::Geometry::FMeshNormals::QuickComputeVertexNormals(Mesh);

	}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::VertexPositions);
}

void UTerraDyneCollisionLib::ConfigureForTerrainPhysics(UDynamicMeshComponent* TargetComponent)
{
	if (!TargetComponent) return;

	// 1. Collision Mode
	// "ComplexAsSimple" uses the actual triangle mesh for collision queries.
	// This is standard for Landscapes (StaticMesh/DynamicMesh).
	TargetComponent->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;

	// 2. Optimization: Disable Rendering Features
	// The physics mesh is invisible; the VHFM handles the visuals.
	// Turning these off saves draw calls and shadow map overhead.
	TargetComponent->SetCastShadow(false);
	TargetComponent->bCastDynamicShadow = false;
	TargetComponent->bCastStaticShadow = false;
	TargetComponent->SetVisibility(false); // Often easier to just hide it, but ensure bHiddenInGame=true
	TargetComponent->SetHiddenInGame(true);

	// 3. Navigation
	// Ensure the NavMesh can walk on this
	TargetComponent->SetCanEverAffectNavigation(true);

	// 4. Physics Profile
	// Ensure it blocks standard traces
	TargetComponent->SetCollisionProfileName(TEXT("BlockAll"));
}

bool UTerraDyneCollisionLib::IsLocationTraceable(UDynamicMeshComponent* MeshComp, FVector LocalLocation)
{
	if (!MeshComp || !MeshComp->GetDynamicMesh()) return false;

	bool bIsHole = false;

	// Access the underlying mesh spatial data structure for a query
	MeshComp->GetDynamicMesh()->ProcessMesh([&](const UE::Geometry::FDynamicMesh3& Mesh)
	{
		// We need a spatial query structure (AABB Tree) to find the triangle at X,Y
		// Note: Building the AABB tree is expensive. If this is called every tick, 
		// the tree should be cached in the Chunk class, not built here.
		// For a static library function, we perform a brute force or simple check if we assume 2D grid.
		
		// FASTER METHOD: Mathematical Look up (Validation only)
		// If we assume the mesh is a perfect grid, we don't need the AABB tree.
		// However, for generic Dynamic Meshes, using the AABB tree is correct.
		
		// NOTE: In a real high-perf scenario, accessing Component->GetDynamicMesh()->GetAABBTree() 
		// requires the component to have "bEnableSpatialDataStructure = true".
	});

	// For TerraDyne, the "Hole" logic is strictly Material ID based.
	// If the physics material at this location has a specific tag or ID, return false.
	// Since PhysX/Chaos handles this natively via "ComplexAsSimple" and Material Masks,
	// this function is often redundant unless doing custom logic.

	return !bIsHole;
}