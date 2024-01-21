// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** Setting describing the session type. This will tell us how to handle the session (value is FString) */
#define SETTING_SESSIONTYPE FName(TEXT("SESSIONTYPE"))
/** Setting describing the session owner's unique id (value is FString) */
#define SETTING_OWNERID FName(TEXT("OWNERID"))
/** Setting describing whether the session uses the reservation system (value is int32 because the Steam Subsystem doesn't support bool queries) */
#define SETTING_USERESERVATIONS FName(TEXT("USERESERVATIONS"))
/** Setting describing whether the session is hidden or not (value is int32 because the Steam Subsystem doesn't support bool queries) */
#define SETTING_HIDDEN FName(TEXT("HIDDEN"))
/** Setting describing the session's display name (value is FString) */
#define SETTING_SERVERNAME FName(TEXT("SERVERNAME"))
/** Setting describing which playlist the session belongs to (value is FString) */
#define SETTING_PLAYLIST FName(TEXT("PLAYLIST"))
/** Setting describing the session's skill level (value is int32) */
#define SETTING_SESSIONELO FName(TEXT("SESSIONELO"))
/** Second key for session elo because query settings can only compare against one session setting (value is int32) */
#define SETTING_SESSIONELO2 FName(TEXT("SESSIONELO2"))
/** Setting describing which players are not allowed to join the session (value is FString of the form "uniqueid1;uniqueid2;uniqueid3") */
#define SETTING_BANNEDPLAYERS FName(TEXT("BANNEDPLAYERS"))
/** Setting describing which level should be opened by the host once the session is created (value is FString) */
#define SETTING_STARTINGLEVEL FName(TEXT("STARTINGLEVEL"))
/** Setting describing the reconnect identifier. Reconnecting clients use this to confirm that they are reconnecting the proper session (value is FString) */
#define SETTING_RECONNECTID FName(TEXT("RECONNECTID"))

/** Kronos log category. */
KRONOS_API DECLARE_LOG_CATEGORY_EXTERN(LogKronos, Log, All);

/** Whether the custom log macros should colorize the output text messages. */
#define HIGHLIGHT_LOGS 1

/** A custom log macro that can colorize the output message. */
#define KRONOS_LOG(Category, Verbosity, Color, Format, ...) \
{ \
	bool bColorizeOutput = HIGHLIGHT_LOGS && PLATFORM_SUPPORTS_COLORIZED_OUTPUT_DEVICE && ELogVerbosity::Verbosity >= ELogVerbosity::Display; \
	if (bColorizeOutput) SET_WARN_COLOR(Color); \
	UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__); \
	if (bColorizeOutput) CLEAR_WARN_COLOR(); \
}

/** A custom conditional log macro that can colorize the output message. */
#define KRONOS_CLOG(Conditional, Category, Verbosity, Color, Format, ...) \
{ \
	bool bColorizeOutput = HIGHLIGHT_LOGS && PLATFORM_SUPPORTS_COLORIZED_OUTPUT_DEVICE && ELogVerbosity::Verbosity >= ELogVerbosity::Display; \
	if (bColorizeOutput) SET_WARN_COLOR(Color); \
	UE_CLOG(Conditional, Category, Verbosity, Format, ##__VA_ARGS__); \
	if (bColorizeOutput) CLEAR_WARN_COLOR(); \
}

/**
 * Implements the runtime module of the Kronos Matchmaking plugin.
 */
class FKronosModule : public IModuleInterface
{
private:

	/** Handle for the plugin validation delegate. */
	FDelegateHandle OnStartGameInstanceDelegateHandle;

	/** Custom console commands of the plugin. */
	TArray<struct IConsoleCommand*> ConsoleCommands;

public:

	/** Register the Kronos runtime module with the engine. */
	virtual void StartupModule() override;

	/** Unregister the Kronos runtime module. */
	virtual void ShutdownModule() override;

private:

	/**
	 * Check the configuration of the plugin and log any errors.
	 * Called when starting the GameInstance.
	 */
	void ValidateModule(class UGameInstance* GameInstance) const;

	/**
	 * [Console command]
	 * Dump current matchmaking settings to the console.
	 */
	void DumpMatchmakingSettings(UWorld* World) const;

	/**
	 * [Console command]
	 * Dump current matchmaking state to the console.
	 */
	void DumpMatchmakingState(UWorld* World) const;

	/**
	 * [Console command]
	 * Dump current party state to the console.
	 */
	void DumpPartyState(UWorld* World) const;

	/**
	 * [Console command]
	 * Dump reservations to the console.
	 */
	void DumpReservations(UWorld* World) const;

	/**
	 * [Console command]
	 * Change the current countdown time in the lobby.
	 */
	void SetLobbyTimer(const TArray<FString>& Args, UWorld* World) const;

	/**
	 * [Console command]
	 * Start the match immediately regardless of lobby state.
	 */
	void LobbyStartMatch(UWorld* World) const;
};
