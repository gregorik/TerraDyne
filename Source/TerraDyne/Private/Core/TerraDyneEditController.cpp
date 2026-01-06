#include "Core/TerraDyneEditController.h"
#include "UI/TerraDyneToolWidget.h"
#include "Core/TerraDyneManager.h"
#include "Core/TerraDyneSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h" 
#include "GameFramework/PlayerInput.h" 

ATerraDyneEditController::ATerraDyneEditController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bIsClicking = false;
	PrimaryActorTick.bCanEverTick = true;
}

void ATerraDyneEditController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (PlayerInput)
	{
		FInputAxisKeyMapping WheelMapping(TEXT("TerraDyneBrushSize"), EKeys::MouseWheelAxis, 1.0f);
		PlayerInput->AddAxisMapping(WheelMapping);

		FInputActionKeyMapping ClickMapping(TEXT("TerraDyneClick"), EKeys::LeftMouseButton);
		PlayerInput->AddActionMapping(ClickMapping);
	}

	InputComponent->BindAction("TerraDyneClick", IE_Pressed, this, &ATerraDyneEditController::OnLeftClickStart);
	InputComponent->BindAction("TerraDyneClick", IE_Released, this, &ATerraDyneEditController::OnLeftClickStop);
	InputComponent->BindAxis("TerraDyneBrushSize", this, &ATerraDyneEditController::OnChangeBrushSize);
}

void ATerraDyneEditController::BeginPlay()
{
	Super::BeginPlay();

	if (UIClass)
	{
		ActiveUI = CreateWidget<UTerraDyneToolWidget>(this, UIClass);
		if (ActiveUI) ActiveUI->AddToViewport();
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ATerraDyneEditController::OnLeftClickStart() { bIsClicking = true; }
void ATerraDyneEditController::OnLeftClickStop() { bIsClicking = false; }

void ATerraDyneEditController::OnChangeBrushSize(float Val)
{
	if (Val == 0.0f || !ActiveUI) return;
	float Delta = Val * 500.0f;
	float NewSize = ActiveUI->BrushRadius + Delta;
	ActiveUI->BrushRadius = FMath::Clamp(NewSize, 100.0f, 20000.0f);
}

// --- HELPER: CUSTOM TRACE (Ignores Player) ---
bool GetTerrainHit(APlayerController* PC, FHitResult& OutHit)
{
	if (!PC) return false;

	FVector WorldLoc, WorldDir;
	if (PC->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
	{
		FVector End = WorldLoc + (WorldDir * 1000000.0f);
		FCollisionQueryParams Params;
		Params.bTraceComplex = true;
		Params.AddIgnoredActor(PC->GetPawn());

		// FIX 4: Trace against WorldStatic to hit the HIDDEN PhysicsMesh
		if (PC->GetWorld()->LineTraceSingleByChannel(OutHit, WorldLoc, End, ECC_WorldStatic, Params))
		{
			return true;
		}
	}
	return false;
}

void ATerraDyneEditController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (IsInputKeyDown(EKeys::LeftMouseButton)) bIsClicking = true; else bIsClicking = false;

	FHitResult Hit;
	// Use Helper
	if (GetTerrainHit(this, Hit))
	{
		if (ActiveUI)
		{
			DrawDebugSphere(GetWorld(), Hit.Location, ActiveUI->BrushRadius, 32, FColor::Green, false, -1.0f, 0, 10.0f);
		}
		if (bIsClicking) PerformToolAction();
	}
}

void ATerraDyneEditController::PerformToolAction()
{
	if (!ActiveUI) return;
	FHitResult Hit;
	if (!GetTerrainHit(this, Hit)) return;

	// ... (Rest of logic same as before, calling Manager->ApplyGlobalBrush) ...
	ATerraDyneManager* Manager = nullptr;
	if (UTerraDyneSubsystem* Sys = GetWorld()->GetSubsystem<UTerraDyneSubsystem>()) Manager = Sys->GetTerrainManager();
	if (!Manager) return;

	float Radius = ActiveUI->BrushRadius;
	float Strength = ActiveUI->BrushStrength;

	switch (ActiveUI->CurrentTool)
	{
	case ETerraDyneToolMode::SculptRaise:
		Manager->ApplyGlobalBrush(Hit.Location, Radius, Strength, false);
		break;
	case ETerraDyneToolMode::SculptLower:
		Manager->ApplyGlobalBrush(Hit.Location, Radius, -Strength, false);
		break;
	case ETerraDyneToolMode::Paint:
		Manager->ApplyGlobalBrush(Hit.Location, Radius, 1.0f, false, ActiveUI->ActiveLayerIndex);
		break;
	}
}