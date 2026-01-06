#include "UI/TerraDyneToolWidget.h"
#include "Core/TerraDyneSubsystem.h"
#include "Core/TerraDyneManager.h"
#include "World/TerraDyneChunk.h"

void UTerraDyneToolWidget::SaveWorld()
{
	if (ATerraDyneManager* Manager = GetWorld()->GetSubsystem<UTerraDyneSubsystem>()->GetTerrainManager())
	{
		// Iterate tracked chunks and save
		// This requires Manager to expose list or save function
		// For demo, we just log:
		UE_LOG(LogTemp, Log, TEXT("UI Request: Save World (Trigger AsyncSaver)"));
	}
}

void UTerraDyneToolWidget::ResetTerrain()
{
	// Logic to re-flatten chunks could go here
}