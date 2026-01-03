using UnrealBuildTool;

public class TerraDyne : ModuleRules
{
	public TerraDyne(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Core dependencies
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"Landscape",
			"RenderCore",
			"RHI",
			"GeometryCore"
		});

		// Plugin dependencies (Must be enabled in .uproject)
		PublicDependencyModuleNames.AddRange(new string[] {
			"GeometryFramework",
			"VirtualHeightfieldMesh"
		});

		// Private dependencies
		PrivateDependencyModuleNames.AddRange(new string[] {
			"GeometryScriptingCore"
		});
	}
}