#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Custom Log Category for Editor/Baking operations
TE_EDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogTerraDyneEditor, Log, All);

class FTerraDyneEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Accessor for the module instance.
	 */
	static inline FTerraDyneEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FTerraDyneEditorModule>("TerraDyneEditor");
	}

	/**
	 * Check if the module is loaded.
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("TerraDyneEditor");
	}
};