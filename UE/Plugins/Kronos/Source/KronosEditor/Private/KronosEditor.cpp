// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosEditor.h"
#include "KronosEditorStyle.h"
#include "KronosConfig.h"
#include "KronosPartyPlayerStart.h"
#include "ISettingsModule.h"
#include "IPlacementModeModule.h"

#define LOCTEXT_NAMESPACE "FKronosEditorModule"

void FKronosEditorModule::StartupModule()
{
	// Initialize the editor style of the plugin.
	FKronosEditorStyle::Initialize();

	// Register plugin settings and placement category.
	RegisterSettings();
	RegisterPlacementCategory();
}

void FKronosEditorModule::ShutdownModule()
{
	// Unregister plugin settings and placement category.
	UnRegisterSettings();
	UnRegisterPlacementCategory();

	// Unregister the editor style of the plugin.
	FKronosEditorStyle::Shutdown();
}

void FKronosEditorModule::RegisterSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		const FText DisplayName = LOCTEXT("KronosConfigName", "Kronos Matchmaking");
		const FText Description = LOCTEXT("KronosConfigDescription", "Configure the Kronos Matchmaking plugin.");
		const TWeakObjectPtr<UObject> SettingsObject = GetMutableDefault<UKronosConfig>(UKronosConfig::StaticClass());

		SettingsModule->RegisterSettings("Project", "Plugins", "KronosConfig", DisplayName, Description, SettingsObject);
	}
}

void FKronosEditorModule::RegisterPlacementCategory()
{
	if (IPlacementModeModule::IsAvailable())
	{
		FPlacementCategoryInfo Info(LOCTEXT("KronosPlaceCategoryName", "Kronos Matchmaking"), FSlateIcon("PluginStyle", "Plugins.TabIcon"), "KronosPlaceCategory", TEXT("PMKronosPlaceCategory"), 45);
		IPlacementModeModule& PlacementModeModule = IPlacementModeModule::Get();
		PlacementModeModule.RegisterPlacementCategory(Info);
		PlacementModeModule.RegisterPlaceableItem(Info.UniqueHandle, MakeShared<FPlaceableItem>(*UActorFactory::StaticClass(), FAssetData(AKronosPartyPlayerStart::StaticClass())));
	}
}

void FKronosEditorModule::UnRegisterSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		SettingsModule->UnregisterSettings("Projects", "Plugins", "KronosConfig");
	}
}

void FKronosEditorModule::UnRegisterPlacementCategory()
{
	if (IPlacementModeModule::IsAvailable())
	{
		IPlacementModeModule& PlacementModeModule = IPlacementModeModule::Get();
		PlacementModeModule.UnregisterPlacementCategory("KronosPlaceCategory");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKronosEditorModule, KronosEditor)