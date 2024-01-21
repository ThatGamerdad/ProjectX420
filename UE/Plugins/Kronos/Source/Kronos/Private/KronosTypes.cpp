// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosTypes.h"
#include "Interfaces/OnlinePresenceInterface.h"

FKronosHostParams::FKronosHostParams()
{
	StartingLevel = FString();
	ServerName = FString();
	Playlist = FString();
	MapName = FString();
	GameMode = FString();
	MaxNumPlayers = 4;
	Elo = 0;
	bShouldAdvertise = true;
	bHidden = false;
	bAllowJoinInProgress = true;
	bIsLanMatch = false;
	bUsesPresence = true;
	bAllowInvites = true;
	bAllowJoinViaPresence = true;
	bUseVoiceChatIfAvailable = false;
	ExtraSessionSettings = TArray<FKronosSessionSetting>();
	BannedPlayers = TArray<FUniqueNetIdRepl>();
}

FKronosMatchmakingParams::FKronosMatchmakingParams()
{
	HostParams = FKronosHostParams();
	Playlist = FString();
	MapName = FString();
	GameMode = FString();
	MaxSearchAttempts = 3;
	MaxSearchResults = 20;
	MinSlotsRequired = 1;
	Elo = 0;
	EloRange = 25;
	EloSearchAttempts = 3;
	EloSearchStep = 25;
	EloRangeBeforeHosting = 75;
	bIsLanQuery = false;
	bSearchPresence = true;
	SpecificSessionQuery = FKronosSpecificSessionQuery();
	ExtraQuerySettings = TArray<FKronosQuerySetting>();
	IgnoredSessions = TArray<FUniqueNetIdRepl>();
}

FKronosMatchmakingParams::FKronosMatchmakingParams(const FKronosHostParams& InHostParams)
{
	HostParams = InHostParams; // This is the only value that matters when creating a new session
	Playlist = FString();
	MapName = FString();
	GameMode = FString();
	MaxSearchAttempts = 1;
	MaxSearchResults = 1;
	MinSlotsRequired = 0;
	Elo = 0;
	EloRange = 0;
	EloSearchAttempts = 1;
	EloSearchStep = 0;
	EloRangeBeforeHosting = MAX_int32;
	bIsLanQuery = false;
	bSearchPresence = false;
	SpecificSessionQuery = FKronosSpecificSessionQuery();
	ExtraQuerySettings = TArray<FKronosQuerySetting>();
	IgnoredSessions = TArray<FUniqueNetIdRepl>();
}

FKronosMatchmakingParams::FKronosMatchmakingParams(const FKronosSearchParams& InSearchParams)
{
	HostParams = FKronosHostParams();
	Playlist = InSearchParams.Playlist;
	MapName = InSearchParams.MapName;
	GameMode = InSearchParams.GameMode;
	MaxSearchAttempts = 1;
	MaxSearchResults = InSearchParams.MaxSearchResults;
	MinSlotsRequired = InSearchParams.MinSlotsRequired;
	Elo = InSearchParams.Elo;
	EloRange = InSearchParams.EloRange;
	EloSearchAttempts = InSearchParams.MaxSearchAttempts;
	EloSearchStep = 0;
	EloRangeBeforeHosting = MAX_int32;
	bIsLanQuery = InSearchParams.bIsLanQuery;
	bSearchPresence = InSearchParams.bSearchPresence;
	SpecificSessionQuery = InSearchParams.SpecificSessionQuery;
	ExtraQuerySettings = InSearchParams.ExtraQuerySettings;
	IgnoredSessions = InSearchParams.IgnoredSessions;
}

FKronosMatchmakingParams::FKronosMatchmakingParams(const FKronosFollowPartyParams& InFollowPartyParams)
{
	HostParams = FKronosHostParams();
	Playlist = FString();
	MapName = FString();
	GameMode = FString();
	MaxSearchAttempts = 1;
	MaxSearchResults = 1;
	MinSlotsRequired = 0;
	Elo = 0;
	EloRange = 0;
	EloSearchAttempts = 5; // Default to 5 attempts, but this is overridden with 'ClientFollowPartyAttempts' config value in code.
	EloSearchStep = 0;
	EloRangeBeforeHosting = MAX_int32;
	bIsLanQuery = InFollowPartyParams.bIsLanQuery;
	bSearchPresence = InFollowPartyParams.bSearchPresence;
	SpecificSessionQuery = InFollowPartyParams.SpecificSessionQuery;
	ExtraQuerySettings = TArray<FKronosQuerySetting>();
	IgnoredSessions = TArray<FUniqueNetIdRepl>();
}

FKronosSearchParams::FKronosSearchParams()
{
	Playlist = FString();
	MapName = FString();
	GameMode = FString();
	MaxSearchAttempts = 3;
	MaxSearchResults = 20;
	MinSlotsRequired = 0;
	Elo = 0;
	EloRange = 25;
	bIsLanQuery = false;
	bSearchPresence = true;
	bSkipEloChecks = false;
	SpecificSessionQuery = FKronosSpecificSessionQuery();
	ExtraQuerySettings = TArray<FKronosQuerySetting>();
	IgnoredSessions = TArray<FUniqueNetIdRepl>();
}

FKronosSearchParams::FKronosSearchParams(const FKronosMatchmakingParams& InMatchmakingParams, const bool bInSkipEloChecks)
{
	Playlist = InMatchmakingParams.Playlist;
	MapName = InMatchmakingParams.MapName;
	GameMode = InMatchmakingParams.GameMode;
	MaxSearchAttempts = InMatchmakingParams.EloSearchAttempts;
	MaxSearchResults = InMatchmakingParams.MaxSearchResults;
	MinSlotsRequired = InMatchmakingParams.MinSlotsRequired;
	Elo = InMatchmakingParams.Elo;
	EloRange = InMatchmakingParams.EloRange;
	bIsLanQuery = InMatchmakingParams.bIsLanQuery;
	bSearchPresence = InMatchmakingParams.bSearchPresence;
	bSkipEloChecks = bInSkipEloChecks;
	SpecificSessionQuery = InMatchmakingParams.SpecificSessionQuery;
	ExtraQuerySettings = InMatchmakingParams.ExtraQuerySettings;
	IgnoredSessions = InMatchmakingParams.IgnoredSessions;
}

FKronosOnlineFriend::FKronosOnlineFriend()
{
	UserId = FUniqueNetIdRepl();
	UserName = TEXT("");
	bIsOnline = false;
	bIsInGame = false;
}

FKronosOnlineFriend::FKronosOnlineFriend(const FOnlineFriend& NativeOnlineFriend)
{
	UserId = NativeOnlineFriend.GetUserId();
	UserName = NativeOnlineFriend.GetDisplayName();
	bIsOnline = NativeOnlineFriend.GetPresence().bIsOnline;
	bIsInGame = NativeOnlineFriend.GetPresence().bIsPlayingThisGame;
}
