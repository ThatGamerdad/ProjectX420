// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Implements the editor module of the Kronos Matchmaking plugin.
 */
class FKronosEditorModule : public IModuleInterface
{
public:

	/** Register the Kronos editor module with the engine. */
	virtual void StartupModule() override;

	/** Unregister the Kronos editor module. */
	virtual void ShutdownModule() override;

private:

	/** Register the plugin's settings window. */
	void RegisterSettings();

	/** Register the plugin's placement mode category. */
	void RegisterPlacementCategory();

	/** Unregister the plugin's settings window. */
	void UnRegisterSettings();

	/** Unregister the plugin's placement mode category. */
	void UnRegisterPlacementCategory();
};
