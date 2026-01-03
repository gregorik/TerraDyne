#include "Factoring/TerraDyneFactory.h"
#include "World/TerraDyneTileData.h"

UTerraDyneTileDataFactory::UTerraDyneTileDataFactory()
{
	// Tell the Editor which class this factory creates
	SupportedClass = UTerraDyneTileData::StaticClass();
	
	// Does it create a new object from scratch? Yes.
	bCreateNew = true;
	
	// Can it edit the object after creation? Yes.
	bEditAfterNew = true;
}

UObject* UTerraDyneTileDataFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// Create the asset
	UTerraDyneTileData* NewAsset = NewObject<UTerraDyneTileData>(InParent, InClass, InName, Flags);
	return NewAsset;
}

bool UTerraDyneTileDataFactory::ShouldShowInNewMenu() const
{
	return true;
}