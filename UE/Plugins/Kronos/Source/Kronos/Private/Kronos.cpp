// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Kronos.h"
#include "KronosConfig.h"
#include "KronosOnlineSession.h"
#include "KronosMatchmakingManager.h"
#include "KronosPartyManager.h"
#include "KronosReservationManager.h"
#include "Lobby/KronosLobbyGameMode.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "KronosGameplayDebugger.h"
#endif

DEFINE_LOG_CATEGORY(LogKronos);

#define LOCTEXT_NAMESPACE "FKronosModule"

void FKronosModule::StartupModule()
{
	// Register the plugin validation delegate.
	OnStartGameInstanceDelegateHandle = FWorldDelegates::OnStartGameInstance.AddRaw(this, &FKronosModule::ValidateModule);
	
#if WITH_GAMEPLAY_DEBUGGER
	// Register a gameplay debugger category for the plugin.
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory(TEXT("Kronos"), IGameplayDebugger::FOnGetCategory::CreateStatic(&FKronosGameplayDebuggerCategory::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGame);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif

	// Register console commands.

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("kronos.DumpMatchmakingSettings"),
		TEXT("Dump matchmaking settings to the console."),
		FConsoleCommandWithWorldDelegate::CreateRaw(this, &FKronosModule::DumpMatchmakingSettings),
		ECVF_Default
	));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("kronos.DumpMatchmakingState"),
		TEXT("Dump matchmaking state to the console."),
		FConsoleCommandWithWorldDelegate::CreateRaw(this, &FKronosModule::DumpMatchmakingState),
		ECVF_Default
	));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("kronos.DumpPartyState"),
		TEXT("Dump party state to the console."),
		FConsoleCommandWithWorldDelegate::CreateRaw(this, &FKronosModule::DumpPartyState),
		ECVF_Default
	));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("kronos.DumpReservations"),
		TEXT("Dump reservations to the console."),
		FConsoleCommandWithWorldDelegate::CreateRaw(this, &FKronosModule::DumpReservations),
		ECVF_Default
	));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("kronos.SetLobbyTimer"),
		TEXT("Change the current countdown time in the lobby. <CountdownTime: int32>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FKronosModule::SetLobbyTimer),
		ECVF_Cheat
	));

	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("kronos.LobbyStartMatch"),
		TEXT("Start the match immediately regardless of lobby state."),
		FConsoleCommandWithWorldDelegate::CreateRaw(this, &FKronosModule::LobbyStartMatch),
		ECVF_Cheat
	));
}

void FKronosModule::ShutdownModule()
{
	// Unregister the plugin validation delegate.
	FWorldDelegates::OnStartGameInstance.Remove(OnStartGameInstanceDelegateHandle);
	
#if WITH_GAMEPLAY_DEBUGGER
	// Unregister the gameplay debugger category of the plugin.
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
		GameplayDebuggerModule.UnregisterCategory("Kronos");
		GameplayDebuggerModule.NotifyCategoriesChanged();
	}
#endif

	// Unregister console commands.
	for (IConsoleCommand* Command : ConsoleCommands)
	{
		IConsoleManager::Get().UnregisterConsoleObject(Command);
	}
}

void FKronosModule::ValidateModule(UGameInstance* GameInstance) const
{
#if WITH_EDITOR
	// Make sure that the online session class is set properly.
	UOnlineSession* OnlineSession = GameInstance->GetOnlineSession();
	if (!OnlineSession->IsA(UKronosOnlineSession::StaticClass()))
	{
		UE_LOG(LogKronos, Error, TEXT("Kronos Validation Error: The GameInstance is not using KronosOnlineSession class!"))
	}

	// Make sure that class references are valid.
	// These may get invalid after direct asset operations, such as moving or deleting .uassets through a file explorer.
	const UKronosConfig* KronosConfig = GetDefault<UKronosConfig>();
	UE_CLOG(!KronosConfig->OnlineSessionClass.Get(), LogKronos, Error, TEXT("Kronos Config: No OnlineSessionClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->MatchmakingPolicyClass.Get(), LogKronos, Error, TEXT("Kronos Config: No MatchmakingPolicyClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->MatchmakingSearchPassClass.Get(), LogKronos, Error, TEXT("Kronos Config: No MatchmakingSearchPassClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->PartyListenerClass.Get(), LogKronos, Error, TEXT("Kronos Config: No PartyListenerClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->PartyHostClass.Get(), LogKronos, Error, TEXT("Kronos Config: No PartyHostClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->PartyClientClass.Get(), LogKronos, Error, TEXT("Kronos Config: No PartyClientClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->PartyStateClass.Get(), LogKronos, Error, TEXT("Kronos Config: No PartyStateClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->PartyPlayerStateClass.Get(), LogKronos, Error, TEXT("Kronos Config: No PartyPlayerStateClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->ReservationListenerClass.Get(), LogKronos, Error, TEXT("Kronos Config: No ReservationListenerClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->ReservationHostClass.Get(), LogKronos, Error, TEXT("Kronos Config: No ReservationHostClass is selected in the plugin's settings."));
	UE_CLOG(!KronosConfig->ReservationClientClass.Get(), LogKronos, Error, TEXT("Kronos Config: No ReservationClientClass is selected in the plugin's settings."));
#endif
}

void FKronosModule::DumpMatchmakingSettings(UWorld* World) const
{
	if (World)
	{
		UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(World);
		MatchmakingManager->DumpMatchmakingSettings();
	}
}

void FKronosModule::DumpMatchmakingState(UWorld* World) const
{
	if (World)
	{
		UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(World);
		MatchmakingManager->DumpMatchmakingState();
	}
}

void FKronosModule::DumpPartyState(UWorld* World) const
{
	if (World)
	{
		UKronosPartyManager* PartyManager = UKronosPartyManager::Get(World);
		PartyManager->DumpPartyState();
	}
}

void FKronosModule::DumpReservations(UWorld* World) const
{
	if (World)
	{
		UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(World);
		ReservationManager->DumpReservations();
	}
}

void FKronosModule::SetLobbyTimer(const TArray<FString>& Args, UWorld* World) const
{
	if (World)
	{
		if (Args.Num() != 0)
		{
			if (World->GetNetMode() == NM_Client)
			{
				UE_LOG(LogKronos, Warning, TEXT("Lobby can only be started by the server."));
				return;
			}

			AKronosLobbyGameMode* LobbyGameMode = World->GetAuthGameMode<AKronosLobbyGameMode>();
			if (!IsValid(LobbyGameMode))
			{
				UE_LOG(LogKronos, Error, TEXT("Failed to get lobby game mode."));
				return;
			}

			int32 CountdownTime = FCString::Atoi(*Args[0]);
			LobbyGameMode->SetLobbyTimer(CountdownTime);
		}
	}
}

void FKronosModule::LobbyStartMatch(UWorld* World) const
{
	if (World)
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogKronos, Warning, TEXT("Lobby can only be started by the server."));
			return;
		}

		AKronosLobbyGameMode* LobbyGameMode = World->GetAuthGameMode<AKronosLobbyGameMode>();
		if (!IsValid(LobbyGameMode))
		{
			UE_LOG(LogKronos, Error, TEXT("Failed to get lobby game mode."));
			return;
		}

		LobbyGameMode->StartMatch();
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FKronosModule, Kronos)