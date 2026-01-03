#include "Core/TerraDyneSubsystem.h"
#include "Core/TerraDyneManager.h"
#include "Grass/TerraDyneGrassSystem.h"
#include "IO/TerraDyneAsyncSaver.h"
#include "Async/TaskGraphInterfaces.h"

void UTerraDyneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("TerraDyneSubsystem: Initializing for World %s"), *GetWorld()->GetName());

	// create the Grass System (Non-UObject worker)
	// We use MakeShared to keep it alive as long as this Subsystem exists
	GrassSystem = MakeShared<FTerraDyneGrassSystem>();
	
	// If the Grass System needs to know about the world (for line traces or spawning), pass it here
	if (GrassSystem.IsValid())
	{
		GrassSystem->Initialize(GetWorld());
	}
}

void UTerraDyneSubsystem::Deinitialize()
{
	// Ensure we don't leave lingering threads or GC references
	FlushPendingTasks();

	if (GrassSystem.IsValid())
	{
		GrassSystem->Shutdown();
		GrassSystem.Reset();
	}

	ActiveManager.Reset();

	UE_LOG(LogTemp, Log, TEXT("TerraDyneSubsystem: Deinitialized."));

	Super::Deinitialize();
}

//--- Manager Registry ---//

void UTerraDyneSubsystem::RegisterManager(ATerraDyneManager* InManager)
{
	if (!InManager) return;

	if (ActiveManager.IsValid() && ActiveManager.Get() != InManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("TerraDyneSubsystem: Multiple Managers detected! Overwriting registration to %s"), *InManager->GetName());
	}

	ActiveManager = InManager;
}

void UTerraDyneSubsystem::UnregisterManager(ATerraDyneManager* InManager)
{
	if (ActiveManager.Get() == InManager)
	{
		ActiveManager.Reset();
	}
}

ATerraDyneManager* UTerraDyneSubsystem::GetTerrainManager() const
{
	if (ActiveManager.IsValid())
	{
		return ActiveManager.Get();
	}
	return nullptr;
}

//--- Grass System Access ---//

TSharedPtr<FTerraDyneGrassSystem> UTerraDyneSubsystem::GetGrassSystem() const
{
	return GrassSystem;
}

//--- IO Safety ---//

void UTerraDyneSubsystem::FlushPendingTasks()
{
	// 1. Cancel/Finish Grass Generation checks
	if (GrassSystem.IsValid())
	{
		GrassSystem->CancelAllTasks();
	}

	// 2. Wait for IO (Thread Pool)
	// If we have critical save games writing to disk, we generally don't want to kill the process.
	// FNonAbandonableTasks running on the thread pool usually finish on their own, 
	// but explicit synchronization can be done here if we tracked specific IO tasks.
	
	// In a production environment, you might use a specific FGraphEvent to track active loads/saves
	// and wait for them:
	// FTaskGraphInterface::Get().WaitUntilTaskCompletes(MyTrackingHandle);
}