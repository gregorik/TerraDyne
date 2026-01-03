#include "TerraDyneModule.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h" // For mapping shader directories if you add custom shaders later
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

// Define the log category
DEFINE_LOG_CATEGORY(LogTerraDyne);

#define LOCTEXT_NAMESPACE "FTerraDyneModule"

void FTerraDyneModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; 
	// the exact timing is specified in the .uplugin file per-module by LoadingPhase.
	
	UE_LOG(LogTerraDyne, Log, TEXT("TerraDyne Runtime Module Started."));

	// OPTIONAL: If Phase 4 evolves to use Custom Shaders (.usf), 
	// this is where you map the virtual directory.
	/*
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("TerraDyne"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/TerraDyne"), PluginShaderDir);
	*/
}

void FTerraDyneModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module. 
	// For modules that support dynamic reloading, we call this function before unloading the module.
	
	UE_LOG(LogTerraDyne, Log, TEXT("TerraDyne Runtime Module Shutting Down."));
}

#undef LOCTEXT_NAMESPACE
	
// Implement the module. 
// "TerraDyne" must match the name in your .uplugin and .Build.cs
IMPLEMENT_MODULE(FTerraDyneModule, TerraDyne)