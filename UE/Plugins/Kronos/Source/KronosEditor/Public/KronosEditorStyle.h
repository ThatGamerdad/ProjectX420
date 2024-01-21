// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**
 * KronosEditorStyle is a custom style set used by the plugin's editor module.
 */
class KRONOSEDITOR_API FKronosEditorStyle
{
private:

	/** Style set used by the plugin. */
	static TSharedPtr<class FSlateStyleSet> Style;

public:

	/**
	 * Initializes the style set.
	 * Called during module startup.
	 */
	static void Initialize();

	/**
	 * Resets the style set.
	 * Called during module shutdown.
	 */
	static void Shutdown();

	/** @return The style set used by the plugin. */
	static const ISlateStyle& Get();

	/** @return The name of the style set used by the plugin. */
	static const FName& GetStyleSetName();
};
