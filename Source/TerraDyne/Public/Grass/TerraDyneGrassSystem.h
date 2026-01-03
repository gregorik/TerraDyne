#pragma once

#include "CoreMinimal.h"
#include "Grass/TerraDyneGrassTypes.h"

// NOTE: No .generated.h include because this is a raw C++ class, not a UObject.

/**
 * FTerraDyneGrassSystem
 *
 * The background scheduler for procedural vegetation.
 * Manages thread-safe access to grass data and dispatches generation tasks.
 */
class TERRADYNE_API FTerraDyneGrassSystem : public TSharedFromThis<FTerraDyneGrassSystem>
{
public:
	// Lifecycle
	void Initialize(UWorld* InWorld);
	void Shutdown();

	/**
	 * Called by Chunks when their data changes (Digging).
	 * Queues a regeneration task for the specific bounds.
	 */
	void RequestRegen(const FBox& WorldBounds);

	/**
	 * Emergency stop for all tasks (e.g. Level Unload).
	 */
	void CancelAllTasks();

private:
	// World reference for line traces / spawning
	TWeakObjectPtr<UWorld> WorldRef;
};