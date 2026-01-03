#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Declare a custom log category for the plugin.
// Usage in code: UE_LOG(LogTerraDyne, Warning, TEXT("Message"));
TERRADYNE_API DECLARE_LOG_CATEGORY_EXTERN(LogTerraDyne, Log, All);

class FTerraDyneModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Accessor for the module instance.
	 * Optional, but helpful for accessing singleton subsystems later.
	 */
	static inline FTerraDyneModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FTerraDyneModule>("TerraDyne");
	}

	/**
	 * Check if the module is loaded.
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("TerraDyne");
	}
};