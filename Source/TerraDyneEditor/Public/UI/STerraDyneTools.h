#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

// Forward Declaration
class ALandscapeProxy;

/**
 * STerraDyneTools
 * 
 * A Slate Widget providing the "TerraDyne Conversion Panel".
 * 
 * Features:
 * - Dropdown to select active Landscapes in the current level.
 * - Text Input for the DataAsset save path.
 * - "Bake" button to trigger UTerraDyneBaker.
 */
class TERRADYNEEDITOR_API STerraDyneTools : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STerraDyneTools)
	{}
	SLATE_END_ARGS()

	/**
	 * Construct the widget.
	 * @param InArgs    Slate arguments.
	 */
	void Construct(const FArguments& InArgs);

private:
	//--- UI Callbacks ---//

	/** Handler for the "Bake" button click. */
	FReply OnBakeClicked();

	/** Handler for changes in the Path Text box. */
	void OnPathTextChanged(const FText& NewText);

	/** Handler for the Landscape ComboBox selection change. */
	void OnLandscapeSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	
	/** Handler for generating widgets in the ComboBox list. */
	TSharedRef<SWidget> OnGenerateComboWidget(TSharedPtr<FString> Item);

	/** Refreshes the list of available landscapes in the level. */
	void RefreshLandscapeList();

	//--- Data Strings for UI ---//
	
	// Helper array to hold names for the ComboBox
	TArray<TSharedPtr<FString>> LandscapeOptions;
	
	// currently selected landscape name
	TSharedPtr<FString> SelectedLandscapeOption;
	
	// Pointer to the actual actor found via the name
	TWeakObjectPtr<ALandscapeProxy> SelectedLandscapeActor;

	// Path to save assets (e.g. "/Game/TerraDyne/Data")
	FString SavePath;

	//--- Widget References ---//
	
	// ComboBox widget ref so we can refresh it programmatically
	TSharedPtr<SComboBox<TSharedPtr<FString>>> LandscapeComboBox;
	
	// Status text block to show success/fail messages
	TSharedPtr<STextBlock> StatusTextBlock;
};