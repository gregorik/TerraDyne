using UnrealBuildTool;

public class TerraDyneEditor : ModuleRules
{
	public TerraDyneEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"TerraDyne",
			"GeometryCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"EditorFramework",
			"AssetTools",
			"AssetRegistry",
			"PropertyEditor",
			"LevelEditor",
			"Landscape", // Required to read source Landscape data
			"RenderCore", // Required for Texture locking/baking
			"RHI"
		});
	}
}