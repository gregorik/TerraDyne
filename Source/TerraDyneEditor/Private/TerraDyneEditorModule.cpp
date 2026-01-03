#include "TerraDyneEditorModule.h"
#include "Modules/ModuleManager.h"

// Define the Log Category declared in the header
DEFINE_LOG_CATEGORY(LogTerraDyneEditor);

#define LOCTEXT_NAMESPACE "FTerraDyneEditorModule"

void FTerraDyneEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; 
	// the exact timing is specified in the .uplugin file per-module by LoadingPhase.
	
	UE_LOG(LogTerraDyneEditor, Log, TEXT("TerraDyne Editor Module Started."));

	// Phase 4 Note:
	// If you want to add custom "Context Menu" actions for UTerraDyneTileData assets 
	// (like "Re-Bake" or "Inspection"), you would register FAssetTypeActions_Base here 
	// using the IAssetTools interface.
	
	/* Example Registration (Optional):
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_TerraDyneTileData()));
	*/
}

void FTerraDyneEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.
	
	UE_LOG(LogTerraDyneEditor, Log, TEXT("TerraDyne Editor Module Shutting Down."));
}

#undef LOCTEXT_NAMESPACE
	
// Implement the module logic
IMPLEMENT_MODULE(FTerraDyneEditorModule, TerraDyneEditor)