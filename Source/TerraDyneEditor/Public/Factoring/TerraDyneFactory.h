#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "TerraDyneFactory.generated.h"

/**
 * UTerraDyneTileDataFactory
 * 
 * Standard Unreal Engine Factory implementation.
 * Registers "TerraDyne Tile Data" in the "Add" / Right-Click menu of the Content Browser.
 */
UCLASS()
class TERRADYNEEDITOR_API UTerraDyneTileDataFactory : public UFactory
{
	GENERATED_BODY()

public:
	UTerraDyneTileDataFactory();

	//--- UFactory Implementation ---//

	/**
	 * Creates the asset when the user clicks 'Create' in the menu.
	 */
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	/**
	 * Determines if this factory can create the new object from scratch.
	 */
	virtual bool ShouldShowInNewMenu() const override;
};