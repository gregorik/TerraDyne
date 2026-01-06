#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TerraDyneEditController.generated.h"

class UTerraDyneToolWidget;

UCLASS()
class TERRADYNE_API ATerraDyneEditController : public APlayerController
{
	GENERATED_BODY()

public:
	ATerraDyneEditController();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UTerraDyneToolWidget> UIClass;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override; // Standard override
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTerraDyneToolWidget> ActiveUI;

	// Mouse State
	bool bIsClicking;

	// Input Handlers
	void OnLeftClickStart();
	void OnLeftClickStop();
	void OnChangeBrushSize(float Val);

	// Logic
	void PerformToolAction();
};