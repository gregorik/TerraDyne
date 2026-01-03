#include "Grass/TerraDyneGrassSystem.h"
#include "Async/AsyncWork.h"
#include "Engine/World.h"

// --- Background Task Definition --- //
class FTerraDyneGrassGenTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FTerraDyneGrassGenTask>;

	TWeakObjectPtr<UWorld> World;
	FBox TargetBounds;

public:
	FTerraDyneGrassGenTask(TWeakObjectPtr<UWorld> InWorld, FBox InBounds)
		: World(InWorld), TargetBounds(InBounds)
	{
	}

	void DoWork()
	{
		// 1. Thread Safety Check
		if (!World.IsValid()) return;

		// 2. Heavy Math: Calculate Grass Positions
		// (In a real implementation, you would raycast against the DynamicMesh 
		// or sample the HeightArray here).

		// 3. Sync back to Game Thread
		// Async(ENamedThreads::GameThread, [...](){ ... Update HISM ... });
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FTerraDyneGrassGenTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};

// --- System Implementation --- //

void FTerraDyneGrassSystem::Initialize(UWorld* InWorld)
{
	WorldRef = InWorld;
}

void FTerraDyneGrassSystem::Shutdown()
{
	CancelAllTasks();
	WorldRef.Reset();
}

void FTerraDyneGrassSystem::RequestRegen(const FBox& WorldBounds)
{
	if (!WorldRef.IsValid()) return;

	// Spawn a background task
	(new FAutoDeleteAsyncTask<FTerraDyneGrassGenTask>(WorldRef, WorldBounds))->StartBackgroundTask();
}

void FTerraDyneGrassSystem::CancelAllTasks()
{
	// Logic to invalidate pending tasks would go here
}