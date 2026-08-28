#pragma once

#include "CoreMinimal.h"

class UWidget;

/**
 * Binds/unbinds click handlers for either a raw UButton or our WBP_Button
 * (which exposes a BlueprintAssignable OnButtonClicked dispatcher).
 */
struct FLoop9WidgetClickBinder
{
	/** Bind Target::FunctionName to Widget's click. Returns true if bound. */
	static bool BindClicked(UWidget* Widget, UObject* Target, FName FunctionName);

	/** Remove Target::FunctionName from Widget's click dispatcher. */
	static void UnbindClicked(UWidget* Widget, UObject* Target, FName FunctionName);

	/** If Widget has a ButtonText FText property (WBP_Button), set it. */
	static void SetButtonText(UWidget* Widget, const FText& InText);

	/** Resolves a real focusable child for raw buttons and composite WBP_Button widgets. */
	static UWidget* ResolveFocusableWidget(UWidget* Widget);

	/**
	 * Pins Widget to the bottom-right of its nearest CanvasPanel ancestor.
	 * Reparents if it currently lives in a centered stack. Size is applied on
	 * the canvas slot so 520x72 continue buttons do not sit in the middle.
	 */
	static bool AlignToCanvasBottomRight(UWidget* Widget, FVector2D PaddingFromCorner, FVector2D Size);
};
