#include "Physics/TerraDyneMeshUtils.h"

// Engine Core
#include "CoreMinimal.h"

// Framework Includes (The high level component)
#include "UDynamicMesh.h" 

// Geometry Scripting (Helper functions)
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/GeometryScriptSelectionTypes.h"

// Core Geometry Definitions (The low level mesh math)
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMeshChangeTracker.h" // <--- CRITICAL: Defines EDynamicMeshChangeType
#include "Math/Vector.h" 
#include "DynamicMesh/DynamicMeshChangeTracker.h"

void UTerraDyneMeshUtils::GenerateTerrainGrid(UDynamicMesh* TargetMesh, float Size, int32 Resolution, int32 MaterialID)
{
	if (!TargetMesh) return;

	TargetMesh->Reset();

	FGeometryScriptPrimitiveOptions Options;
	Options.PolygroupMode = EGeometryScriptPrimitivePolygroupMode::PerFace;
	Options.UVMode = EGeometryScriptPrimitiveUVMode::Uniform;

	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(
		TargetMesh,
		Options,
		FTransform::Identity,
		Size,
		Size,
		Resolution,
		Resolution
	);

	if (MaterialID != 0)
	{
		FGeometryScriptMeshSelection AllSelection;
		UGeometryScriptLibrary_MeshMaterialFunctions::SetMaterialIDForMeshSelection(TargetMesh, AllSelection, MaterialID);
	}
}

void UTerraDyneMeshUtils::UpdateHoleAtLocation(UDynamicMesh* TargetMesh, FVector LocalCenter, float Radius, int32 HoleMaterialID, bool bRefill)
{
	if (!TargetMesh) return;

	// Use low-level EditMesh for direct FDynamicMesh3 access
	TargetMesh->EditMesh([&](UE::Geometry::FDynamicMesh3& Mesh)
		{
			// 1. Ensure Attributes Exist
			if (!Mesh.HasAttributes())
			{
				Mesh.EnableAttributes();
			}

			// 2. Get the Material ID Attribute Layer
			UE::Geometry::FDynamicMeshMaterialAttribute* MatAttrib = Mesh.Attributes()->GetMaterialID();
			if (!MatAttrib)
			{
				Mesh.Attributes()->EnableMaterialID();
				MatAttrib = Mesh.Attributes()->GetMaterialID();
			}

			double RadiusSq = Radius * Radius;
			FVector2D Center2D(LocalCenter.X, LocalCenter.Y);

			int32 TargetID = bRefill ? 0 : HoleMaterialID;

			for (int32 TriID : Mesh.TriangleIndicesItr())
			{
				FVector3d A, B, C;
				Mesh.GetTriVertices(TriID, A, B, C);

				FVector2D Centroid(
					(A.X + B.X + C.X) / 3.0,
					(A.Y + B.Y + C.Y) / 3.0
				);

				if (FVector2D::DistSquared(Centroid, Center2D) <= RadiusSq)
				{
					MatAttrib->SetValue(TriID, TargetID);
				}
			}

		},
		EDynamicMeshChangeType::AttributeEdit,
		EDynamicMeshAttributeChangeFlags::TriangleGroups); // <-- FIXED: Use TriangleGroups for material ID changes
}

void UTerraDyneMeshUtils::RecomputeNormals(UDynamicMesh* TargetMesh)
{
	if (!TargetMesh) return;

	UGeometryScriptLibrary_MeshNormalsFunctions::ComputeSplitNormals(
		TargetMesh,
		FGeometryScriptSplitNormalsOptions(),
		FGeometryScriptCalculateNormalsOptions()
	);
}