// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FKronosEditorStyle::Style = nullptr;

void FKronosEditorStyle::Initialize()
{
	if (Style.IsValid())
	{
		return;
	}

	const FVector2D Icon16x16 = FVector2D(16.0f, 16.0f);
	const FVector2D Icon64x64 = FVector2D(64.0f, 64.0f);

	const FString PluginResources = IPluginManager::Get().FindPlugin(TEXT("Kronos"))->GetBaseDir() / TEXT("Resources");
	const FString EditorResources = FPaths::EngineContentDir() / TEXT("Editor/Slate");

	Style = MakeShareable(new FSlateStyleSet("KronosEditorStyle"));
	Style->SetContentRoot(PluginResources);
	Style->SetCoreContentRoot(EditorResources);

	Style->Set("ClassIcon.KronosPartyPlayerStart", new FSlateImageBrush(Style->RootToCoreContentDir(TEXT("Icons/AssetIcons/PlayerStart_16x.png")), Icon16x16));
	Style->Set("ClassThumbnail.KronosPartyPlayerStart", new FSlateImageBrush(Style->RootToCoreContentDir(TEXT("Icons/AssetIcons/Actor_64x.png")), Icon64x64));

	FSlateStyleRegistry::RegisterSlateStyle(*Style.Get());
}

void FKronosEditorStyle::Shutdown()
{
	if (Style.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Style.Get());
		Style.Reset();
	}
}

const ISlateStyle& FKronosEditorStyle::Get()
{
	return *(Style.Get());
}

const FName& FKronosEditorStyle::GetStyleSetName()
{
	return Style->GetStyleSetName();
}
