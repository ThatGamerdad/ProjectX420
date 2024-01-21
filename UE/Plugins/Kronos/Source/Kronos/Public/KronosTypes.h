// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kronos.h"
#include "Online/CoreOnline.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/OnlineReplStructs.h"
#include "Engine/EngineTypes.h"
#include "KronosTypes.generated.h"

/**
 * Possible user authentication states (while in-progress).
 */
UENUM(BlueprintType)
enum class EKronosUserAuthState : uint8
{
	/** No authentication is in progress. */
	NotAuthenticating,

	/** Waiting for platform login. */
	PlatformLogin,

	/** Reading user files from cloud. */
	ReadUserFiles,

	/** Waiting for custom auth tasks to complete (implemented by end user of plugin). */
	CustomAuthTask
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosUserAuthState Value)
{
	switch (Value)
	{
	case EKronosUserAuthState::NotAuthenticating:
		return TEXT("NotAuthenticating");
	case EKronosUserAuthState::PlatformLogin:
		return TEXT("PlatformLogin");
	case EKronosUserAuthState::ReadUserFiles:
		return TEXT("ReadUserFiles");
	case EKronosUserAuthState::CustomAuthTask:
		return TEXT("CustomAuthTask");
	default:
		return TEXT("");
	}
}

/**
 * Possible user authentication request complete results.
 */
UENUM(BlueprintType)
enum class EKronosUserAuthCompleteResult : uint8
{
	/** Authentication successful. */
	Success,

	/** Login status with the Online Subsystem was lost. */
	PlatformLoginStatusLost,

	/** Failed to login with the Online Subsystem. */
	PlatformLoginFailed,

	/** Failed to read user files from the Online Subsystem's cloud storage. */
	ReadUserFilesFailed,

	/** Failed to complete custom auth task (implemented by end user of plugin). */
	CustomAuthTaskFailed,

	/** Unknown error. */
	UnknownError
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosUserAuthCompleteResult Value)
{
	switch (Value)
	{
	case EKronosUserAuthCompleteResult::Success:
		return TEXT("Success");
	case EKronosUserAuthCompleteResult::PlatformLoginStatusLost:
		return TEXT("PlatformLoginStatusLost");
	case EKronosUserAuthCompleteResult::PlatformLoginFailed:
		return TEXT("PlatformLoginFailed");
	case EKronosUserAuthCompleteResult::ReadUserFilesFailed:
		return TEXT("ReadUserFilesFailed");
	case EKronosUserAuthCompleteResult::CustomAuthTaskFailed:
		return TEXT("CustomAuthTaskFailed");
	case EKronosUserAuthCompleteResult::UnknownError:
		return TEXT("UnknownError");
	default:
		return TEXT("");
	}
}

/**
 * Blueprint wrapper around a single session setting.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosSessionSetting
{
	GENERATED_BODY()

	/** Settings key. */
	FName Key;

	/** Settings value. */
	FVariantData Data;

	/** How the setting is advertised with the backend. */
	EOnlineDataAdvertisementType::Type AdvertisementType;

	/** Default constructor. */
	FKronosSessionSetting() :
		Key(NAME_None),
		Data(FVariantData()),
		AdvertisementType(EOnlineDataAdvertisementType::DontAdvertise)
	{}

	/** Constructor starting with an already initialized variant data. */
	FKronosSessionSetting(const FName InKey, const FVariantData& InData, const EOnlineDataAdvertisementType::Type InType) :
		Key(InKey),
		Data(InData),
		AdvertisementType(InType)
	{}

	/** Constructor starting with an already initialized value. */
	template<typename ValueType>
	FKronosSessionSetting(const FName InKey, const ValueType InValue, const EOnlineDataAdvertisementType::Type InType) :
		Key(InKey),
		Data(FVariantData(InValue)),
		AdvertisementType(InType)
	{}

	/** @return Whether the session setting is valid or not. */
	bool IsValid() const
	{
		return Key != NAME_None && Data.GetType() != EOnlineKeyValuePairDataType::Empty;
	}
};

/**
 * Blueprint wrapper around a single session query setting.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosQuerySetting
{
	GENERATED_BODY()

	/** Settings key. */
	FName Key;

	/** Settings value. */
	FVariantData Data;

	/** How the setting is compared on the backend. */
	EOnlineComparisonOp::Type ComparisonOp;

	/** Default constructor. */
	FKronosQuerySetting() :
		Key(NAME_None),
		Data(FVariantData()),
		ComparisonOp(EOnlineComparisonOp::Equals)
	{}

	/** Constructor starting with an already initialized variant data. */
	FKronosQuerySetting(const FName InKey, const FVariantData& InData, const EOnlineComparisonOp::Type InType) :
		Key(InKey),
		Data(InData),
		ComparisonOp(InType)
	{}

	/** Constructor starting with an already initialized value. */
	template<typename ValueType>
	FKronosQuerySetting(const FName InKey, const ValueType InValue, const EOnlineComparisonOp::Type InType) :
		Key(InKey),
		Data(FVariantData(InValue)),
		ComparisonOp(InType)
	{}

	/** @return Whether the query setting is valid or not. */
	bool IsValid() const
	{
		bool bComparisonOpValid = ComparisonOp < EOnlineComparisonOp::Near; // Don't accept Near, In, NotIn types.
		return Key != NAME_None && Data.GetType() != EOnlineKeyValuePairDataType::Empty && bComparisonOpValid;
	}

	/**
	 * Compare the query setting to a given session setting. This is used when we are auto-filtering search results.
	 * Assumes that the given session setting has the same value type as the query setting.
	 */
	template<typename ValueType>
	bool CompareAgainst(const FOnlineSessionSetting* SessionSetting) const
	{
		if (!SessionSetting)
		{
			UE_LOG(LogKronos, Error, TEXT("FKronosQuerySetting comparison was called with nullptr!"));
			return false;
		}

		ValueType QueryValue;
		Data.GetValue(QueryValue);

		ValueType SessionValue;
		SessionSetting->Data.GetValue(SessionValue);

		switch (ComparisonOp)
		{
		case EOnlineComparisonOp::Equals:
			return SessionValue == QueryValue;
		case EOnlineComparisonOp::NotEquals:
			return SessionValue != QueryValue;
		case EOnlineComparisonOp::GreaterThan:
			return SessionValue > QueryValue;
		case EOnlineComparisonOp::GreaterThanEquals:
			return SessionValue >= QueryValue;
		case EOnlineComparisonOp::LessThan:
			return SessionValue < QueryValue;
		case EOnlineComparisonOp::LessThanEquals:
			return SessionValue <= QueryValue;
		default:
			return false;
		}
	}
};

/**
 * Blueprint wrapper around the native EOnlineComparisonOp enum.
 */
UENUM(BlueprintType)
enum class EKronosQueryComparisonOp : uint8
{
	Equals,

	NotEquals,

	GreaterThan,

	GreaterThanEquals, 

	LessThan,

	LessThanEquals
};

/**
 * Parameters used when creating a session.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosHostParams
{
	GENERATED_BODY()

	/**
	 * The map that should be loaded by the host once the session is created.
	 * Can be given as a short package name ("MyMap"), but long package names are preferred ("/Game/Maps/MyMap").
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString StartingLevel;

	/**
	 * Name of the server. This is purely cosmetic information.
	 * If empty, the default server name will be used which is "{PlayerName}'s Session" by default.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString ServerName;

	/**
	 * Name of the playlist this match belongs to. This is purely cosmetic information.
	 * Can be used for filtering purposes during matchmaking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString Playlist;

	/**
	 * Name of the map that this match is being played on. This is purely cosmetic information.
	 * Can be used for filtering purposes during matchmaking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString MapName;

	/**
	 * Name of the game mode that this match uses. This is purely cosmetic information.
	 * Can be used for filtering purposes during matchmaking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString GameMode;

	/** Max session capacity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 MaxNumPlayers;

	/** Skill rating of the session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 Elo;

	/** Should the session be publicly advertised. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bShouldAdvertise;

	/**
	 * Should the session be hidden. Hidden sessions can only be found by specific session queries.
	 * This is useful if you want to have "private" matches, but your online subsystem requires sessions to be advertised for some reason.
	 * Hidden sessions must be advertised!
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bHidden;

	/** Should the session allow players to join once the game has started. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bAllowJoinInProgress;

	/** Should the session be LAN only and not be visible to external players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bIsLanMatch;

	/** Should the session use presence information. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bUsesPresence;

	/** Should the session allow player invitations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bAllowInvites;

	/** Should the session allow players to join via presence information. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bAllowJoinViaPresence;

	/** Should players in the session create (and auto join) a voice chat room, if the platform supports it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bUseVoiceChatIfAvailable;

	/**
	 * List of extra session settings to be used when creating the session.
	 * During matchmaking you can set filters against these session settings via ExtraQuerySettings.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Default")
	TArray<FKronosSessionSetting> ExtraSessionSettings;

	/** List of players who are not allowed to join the session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Default")
	TArray<FUniqueNetIdRepl> BannedPlayers;

	/**
	 * Specific session settings to use when creating the session.
	 * Can be used to recreate a session from previous settings.
	 * The session owner id will be fixed automatically when creating the session.
	 */
	TOptional<FOnlineSessionSettings> SessionSettingsOverride;

	/** Default constructor. */
	FKronosHostParams();

	/** @return Whether the current parameter configuration is valid or not. */
	bool IsValid(const bool bLogErrors = true) const
	{
		// Don't validate if override settings are given.
		if (HasSessionSettingsOverride())
		{
			return true;
		}

		bool bIsValid = true;

		if (StartingLevel.IsEmpty())
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("StartingLevel of FKronosHostParams is invalid!"));
			bIsValid = false;
		}

		if (MaxNumPlayers <= 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("MaxNumPlayers of FKronosHostParams is invalid! Value must be greater than zero."));
			bIsValid = false;
		}

		if (Elo < 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("Elo of FKronosHostParams is invalid! Value shouldn't be negative."));
			bIsValid = false;
		}

		for (const FKronosSessionSetting& ExtraSetting : ExtraSessionSettings)
		{
			if (!ExtraSetting.IsValid())
			{
				UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("ExtraSessionSetting '%s' of FKronosHostParams is invalid!"), *ExtraSetting.Key.ToString());
				bIsValid = false;
			}
		}

		return bIsValid;
	}

	/** @return Whether session settings are overriden by the user. */
	FORCEINLINE bool HasSessionSettingsOverride() const
	{
		return SessionSettingsOverride.IsSet();
	}
};

/**
 * Possible query types of a specific session query. Tells us which online search method should be used to find the desired session.
 * Also tells us what the given unique id stored in the specific session query actually is (friend id, session id, or a session owner's id).
 */
UENUM(BlueprintType)
enum class EKronosSpecificSessionQueryType : uint8
{
	/** Invalid query type. */
	Unspecified,

	/** Find the session using an online friend's unique id. */
	FriendId,

	/** Find the session using the session's unique id. */
	SessionId,

	/** Find the session using the session owner's unique id. */
	SessionOwnerId
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosSpecificSessionQueryType Value)
{
	switch (Value)
	{
	case EKronosSpecificSessionQueryType::Unspecified:
		return TEXT("Unspecified");
	case EKronosSpecificSessionQueryType::FriendId:
		return TEXT("FriendId");
	case EKronosSpecificSessionQueryType::SessionId:
		return TEXT("SessionId");
	case EKronosSpecificSessionQueryType::SessionOwnerId:
		return TEXT("SessionOwnerId");
	default:
		return TEXT("");
	}
}

/**
 * Parameters to be used when we want the matchmaking to search for a specific session.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosSpecificSessionQuery
{
	GENERATED_BODY()

	/** Query type. Tells us which online search method should be used, and what the given unique id is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	EKronosSpecificSessionQueryType Type;

	/** Unique id to be used when searching for the desired session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FUniqueNetIdRepl UniqueId;

	/** Default constructor. */
	FKronosSpecificSessionQuery() :
		Type(EKronosSpecificSessionQueryType::Unspecified),
		UniqueId(FUniqueNetIdRepl())
	{}

	/** @return Whether the current parameter configuration is valid or not. */
	bool IsValid() const
	{
		return Type != EKronosSpecificSessionQueryType::Unspecified && UniqueId.IsValid();
	}
};

/**
 * Matchmaking parameters.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosMatchmakingParams
{
	GENERATED_BODY()

	/** Parameters to be used when the matchmaking is creating a new session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FKronosHostParams HostParams;

	/** Playlist name to matchmake for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString Playlist;

	/**
	 * Map name to matchmake for. Unfortunately multiple map names aren't supported.
	 * For multiple maps, consider using the playlist name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString MapName;

	/**
	 * Game mode to matchmake for. Unfortunately multiple game modes aren't supported.
	 * For multiple game modes, consider using the playlist name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString GameMode;

	/** Max number of search attempts before hosting a session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 MaxSearchAttempts;

	/** Max number of search results per search pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 MaxSearchResults;

	/**
	 * Minimum number of free slots a session must have.
	 * This should reflect the current party size.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 MinSlotsRequired;

	/** Skill rating to matchmake for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 Elo;

	/** Skill rating search range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 EloRange;

	/** Number of search attempts around the given skill rating before increasing the search range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 EloSearchAttempts;

	/** Search range increase after an unsuccessful search pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 EloSearchStep;

	/** Max search range before hosting a session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 EloRangeBeforeHosting;

	/** Whether to search for LAN sessions or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bIsLanQuery;

	/** Whether to search for presence sessions or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bSearchPresence;

	/** If set the matchmaking will search for a specific session based on the given parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Default")
	FKronosSpecificSessionQuery SpecificSessionQuery;

	/** List of extra query settings to be used when searching for sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Default")
	TArray<FKronosQuerySetting> ExtraQuerySettings;

	/** List of sessions to ignore. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Default")
	TArray<FUniqueNetIdRepl> IgnoredSessions;

	/** Default constructor. */
	FKronosMatchmakingParams();

	/** Constructor from host params. */
	FKronosMatchmakingParams(const FKronosHostParams& InHostParams);

	/** Constructor from search params. */
	FKronosMatchmakingParams(const struct FKronosSearchParams& InSearchParams);

	/** Constructor from follow party params. */
	FKronosMatchmakingParams(const struct FKronosFollowPartyParams& InFollowPartyParams);

	/** @return Whether the current parameter configuration is valid or not. */
	bool IsValid(const bool bLogErrors = true) const
	{
		bool bIsValid = true;

		if (MaxSearchAttempts <= 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("MaxSearchAttempts of FKronosMatchmakingParams is invalid! Value must be greater than zero."));
			bIsValid = false;
		}

		if (MaxSearchResults <= 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("MaxSearchResults of FKronosMatchmakingParams is invalid! Value must be greater than zero."));
			bIsValid = false;
		}

		if (MinSlotsRequired < 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("MinSlotsRequired of FKronosMatchmakingParams is invalid! Value shouldn't be negative."));
			bIsValid = false;
		}

		if (Elo < 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("Elo of FKronosMatchmakingParams is invalid! Value shouldn't be negative."));
			bIsValid = false;
		}

		if (EloRange < 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("EloRange of FKronosMatchmakingParams is invalid! Value shouldn't be negative."));
			bIsValid = false;
		}

		if (EloSearchAttempts <= 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("EloSearchAttempts of FKronosMatchmakingParams is invalid! Value must be greater than zero."));
			bIsValid = false;
		}

		if (EloSearchStep < 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("EloSearchStep of FKronosMatchmakingParams is invalid! Value shouldn't be negative."));
			bIsValid = false;
		}

		if (EloRangeBeforeHosting <= 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("EloRangeBeforeHosting of FKronosMatchmakingParams is invalid! Value must be greater than zero."));
			bIsValid = false;
		}

		if (IsSpecificSessionQuery() && !SpecificSessionQuery.IsValid())
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("SpecificSessionQuery of FKronosMatchmakingParams is invalid!"));
			bIsValid = false;
		}

		for (const FKronosQuerySetting& ExtraSetting : ExtraQuerySettings)
		{
			if (!ExtraSetting.IsValid())
			{
				UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("ExtraQuerySetting '%s' of FKronosMatchmakingParams is invalid!"), *ExtraSetting.Key.ToString());
				bIsValid = false;
			}
		}

		return bIsValid;
	}

	/** @return Whether the matchmaking is for a specific session or not. */
	bool IsSpecificSessionQuery() const
	{
		return SpecificSessionQuery.Type != EKronosSpecificSessionQueryType::Unspecified;
	}
};

/**
 * Search pass parameters.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosSearchParams
{
	GENERATED_BODY()

	/** Playlist name to search for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString Playlist;

	/**
	 * Map name to search for. Unfortunately multiple map names aren't supported.
	 * For multiple maps, consider using the playlist name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString MapName;

	/**
	 * Game mode to search for. Unfortunately multiple game modes aren't supported.
	 * For multiple game modes, consider using the playlist name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString GameMode;

	/** Max number of search attempts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 MaxSearchAttempts;

	/** Max number of sessions per search attempt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 MaxSearchResults;

	/** Minimum number of free slots a session must have. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 MinSlotsRequired;

	/** Skill rating to search for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 Elo;

	/** Skill rating search range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 EloRange;

	/** Whether to search for LAN sessions or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bIsLanQuery;

	/** Whether to search for presence sessions or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bSearchPresence;

	/** Whether to skip skill rating checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bSkipEloChecks;

	/** If set the search will be for a specific session based on the given parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Default")
	FKronosSpecificSessionQuery SpecificSessionQuery;

	/** List of extra query settings to be used when searching for sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Default")
	TArray<FKronosQuerySetting> ExtraQuerySettings;

	/** List of sessions to ignore. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Default")
	TArray<FUniqueNetIdRepl> IgnoredSessions;

	/** Default constructor. */
	FKronosSearchParams();

	/** Constructor from matchmaking params. */
	FKronosSearchParams(const FKronosMatchmakingParams& InMatchmakingParams, const bool bInSkipEloChecks);

	/** @return Whether the current parameter configuration is valid or not. */
	bool IsValid(const bool bLogErrors = true) const
	{
		bool bIsValid = true;

		if (MaxSearchAttempts <= 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("MaxSearchAttempts of FKronosSearchParams is invalid! Value must be greater than zero."));
			bIsValid = false;
		}

		if (MaxSearchResults <= 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("MaxSearchResults of FKronosSearchParams is invalid! Value must be greater than zero."));
			bIsValid = false;
		}

		if (MinSlotsRequired < 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("MinSlotsRequired of FKronosSearchParams is invalid! Value shouldn't be negative."));
			bIsValid = false;
		}

		if (Elo < 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("Elo of FKronosSearchParams is invalid! Value shouldn't be negative."));
			bIsValid = false;
		}

		if (EloRange < 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("EloRange of FKronosSearchParams is invalid! Value shouldn't be negative."));
			bIsValid = false;
		}

		if (IsSpecificSessionQuery() && !SpecificSessionQuery.IsValid())
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("SpecificSessionQuery of FKronosSearchParams is invalid!"));
			bIsValid = false;
		}

		for (const FKronosQuerySetting& ExtraSetting : ExtraQuerySettings)
		{
			if (!ExtraSetting.IsValid())
			{
				UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("ExtraQuerySetting '%s' of FKronosSearchParams is invalid!"), *ExtraSetting.Key.ToString());
				bIsValid = false;
			}
		}

		return bIsValid;
	}

	/** @return Whether the search is for a specific session or not. */
	bool IsSpecificSessionQuery() const
	{
		return SpecificSessionQuery.Type != EKronosSpecificSessionQueryType::Unspecified;
	}
};

/*
 * Possible party roles.
 */
UENUM(BlueprintType)
enum class EKronosPartyRole : uint8
{
	/** No party. */
	NoParty,

	/** In party as client. */
	PartyClient,

	/** In party as host. */
	PartyHost
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosPartyRole Value)
{
	switch (Value)
	{
	case EKronosPartyRole::NoParty:
		return TEXT("NoParty");
	case EKronosPartyRole::PartyClient:
		return TEXT("PartyClient");
	case EKronosPartyRole::PartyHost:
		return TEXT("PartyHost");
	default:
		return TEXT("");
	}
}

/*
 * Information about the last party that we were a part of.
 * Can be used to reconnect to the last party.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosLastPartyInfo
{
	GENERATED_BODY()

	/** Our role in the last party. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Default")
	EKronosPartyRole LastPartyRole;

	/** UniqueId of the last party host. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Default")
	FUniqueNetIdRepl LastPartyHostPlayerId;

	/**
	 * UniqueId of the last party session.
	 * Used as the reconnect identifier to ensure that clients connect the proper session.
	 */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Default")
	FString LastPartySessionId;

	/**
	 * Number of players in the last party.
	 * Can be used to check if all players have reconnected or not.
	 */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Default")
	int32 LastPartyPlayerCount;

	/**
	 * Session settings of the last party.
	 * Used when recreating the party.
	 */
	FOnlineSessionSettings LastPartySettings;

	/** Default constructor. */
	FKronosLastPartyInfo() :
		LastPartyRole(EKronosPartyRole::NoParty),
		LastPartyHostPlayerId(FUniqueNetIdRepl()),
		LastPartySessionId(FString()),
		LastPartyPlayerCount(0),
		LastPartySettings(FOnlineSessionSettings())
	{}

	/** @return Whether the party information is valid or not. */
	bool IsValid() const
	{
		return LastPartyRole != EKronosPartyRole::NoParty && LastPartyHostPlayerId.IsValid()
			&& !LastPartySessionId.IsEmpty() && LastPartySessionId != TEXT("InvalidSession");
	}

	/** @return Reconnect id to use when recreating or rejoining the party. */
	FString GetReconnectId() const
	{
		return LastPartySessionId;
	}

	/** @return Number of players we are expecting to reconnect the party. */
	int32 GetNumExpectedPlayers() const
	{
		return LastPartyPlayerCount;
	}
};

/**
 * Parameters to be used for matchmaking when we are following the party to a session.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosFollowPartyParams
{
	GENERATED_BODY()

	/** Specific session query to be used for the matchmaking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FKronosSpecificSessionQuery SpecificSessionQuery;

	/** Whether to search for LAN sessions or not. Only ever used if we are trying to find the session via a regular search. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bIsLanQuery;

	/** Whether to search for presence sessions or not. Only ever used if we are trying to find the session via a regular search. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bSearchPresence;

	/** Whether the party leader is hosting the session for the party. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bPartyLeaderCreatingSession;

	/** Default constructor. */
	FKronosFollowPartyParams() :
		SpecificSessionQuery(FKronosSpecificSessionQuery()),
		bIsLanQuery(false),
		bSearchPresence(false),
		bPartyLeaderCreatingSession(false)
	{}

	/** @return Whether the current parameter configuration is valid or not. */
	bool IsValid() const
	{
		return SpecificSessionQuery.IsValid();
	}
};

/**
 * Parameters of an existing session.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosSessionSettings
{
	GENERATED_BODY()

	/** Name of the server. This is purely cosmetic information. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString ServerName;

	/**
	 * Name of the playlist this match belongs to. This is purely cosmetic information.
	 * Can be used for filtering purposes during matchmaking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString Playlist;

	/**
	 * Name of the map that this match is being played on. This is purely cosmetic information.
	 * Can be used for filtering purposes during matchmaking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString MapName;

	/**
	 * Name of the game mode that this match uses. This is purely cosmetic information.
	 * Can be used for filtering purposes during matchmaking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FString GameMode;

	/** Max session capacity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 MaxNumPlayers;

	/** Skill rating of the session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	int32 Elo;

	/** Whether the session is publicly advertised or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bShouldAdvertise;

	/** Whether the session is hidden or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bHidden;

	/** Whether players are allowed to join once the game has started. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bAllowJoinInProgress;

	/** Whether the session is a LAN session or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bIsLanMatch;

	/** Whether the session uses presence information or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bUsesPresence;

	/** Whether session invitations are allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bAllowInvites;

	/** Whether joining via presence information is allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bAllowJoinViaPresence;

	/** Should players in the session create (and auto join) a voice chat room, if the platform supports it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool bUseVoiceChatIfAvailable;

	/** Default constructor. */
	FKronosSessionSettings() :
		ServerName(FString()),
		Playlist(FString()),
		MapName(FString()),
		GameMode(FString()),
		MaxNumPlayers(0),
		Elo(0),
		bShouldAdvertise(false),
		bHidden(false),
		bAllowJoinInProgress(false),
		bIsLanMatch(false),
		bUsesPresence(false),
		bAllowInvites(false),
		bAllowJoinViaPresence(false),
		bUseVoiceChatIfAvailable(false)
	{}

	/** Constructor from native session settings. */
	FKronosSessionSettings(const FOnlineSessionSettings& InSessionSettings) :
		ServerName(FString()),
		Playlist(FString()),
		MapName(FString()),
		GameMode(FString()),
		MaxNumPlayers(InSessionSettings.NumPublicConnections),
		Elo(0),
		bShouldAdvertise(InSessionSettings.bShouldAdvertise),
		bHidden(false),
		bAllowJoinInProgress(InSessionSettings.bAllowJoinInProgress),
		bIsLanMatch(InSessionSettings.bIsLANMatch),
		bUsesPresence(InSessionSettings.bUsesPresence),
		bAllowInvites(InSessionSettings.bAllowInvites),
		bAllowJoinViaPresence(InSessionSettings.bAllowJoinViaPresence),
		bUseVoiceChatIfAvailable(InSessionSettings.bUseLobbiesVoiceChatIfAvailable)
	{
		InSessionSettings.Get(SETTING_SERVERNAME, ServerName);
		InSessionSettings.Get(SETTING_PLAYLIST, Playlist);
		InSessionSettings.Get(SETTING_MAPNAME, MapName);
		InSessionSettings.Get(SETTING_GAMEMODE, GameMode);
		InSessionSettings.Get(SETTING_SESSIONELO, Elo);

		// This setting is actually an int32 because the Steam Subsystem doesn't support bool queries.
		// We'll just convert it back to a bool.
		int32 SessionHidden = 0;
		InSessionSettings.Get(SETTING_HIDDEN, SessionHidden);
		bHidden = SessionHidden != 0;
	}
};

/**
 * Blueprint wrapper around the native FOnlineSessionSearchResult class.
 * Exposes native search results to blueprints and implements some helper functions.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosSearchResult
{
	GENERATED_BODY()

	/** The native search result returned by the online subsystem. */
	FOnlineSessionSearchResult OnlineResult;

	/** Default constructor. */
	FKronosSearchResult() :
		OnlineResult(FOnlineSessionSearchResult())
	{}

	/** Constructor from a native search result. */
	FKronosSearchResult(const FOnlineSessionSearchResult& InSessionResult) :
		OnlineResult(InSessionResult)
	{}

	/** @return Whether the search result is valid or not. */
	bool IsValid() const
	{
		return OnlineResult.IsValid();
	}

	/** @return Whether the given player is banned from this session or not. */
	bool IsPlayerBannedFromSession(FUniqueNetIdRepl& PlayerId) const
	{
		TArray<FUniqueNetIdRepl> PlayerArray = TArray<FUniqueNetIdRepl>();
		PlayerArray.Add(PlayerId);

		return IsPlayerBannedFromSession(PlayerArray);
	}

	/** @return Whether any of the given players are banned from this session or not. */
	bool IsPlayerBannedFromSession(const TArray<FUniqueNetIdRepl>& PlayerIds) const
	{
		FString BannedPlayers;
		GetSessionSetting(SETTING_BANNEDPLAYERS, BannedPlayers);

		if (!BannedPlayers.IsEmpty())
		{
			TArray<FString> BannedPlayersArray;
			if (BannedPlayers.ParseIntoArray(BannedPlayersArray, TEXT(";")) > 0)
			{
				for (const FUniqueNetIdRepl& PlayerId : PlayerIds)
				{
					if (BannedPlayersArray.Contains(PlayerId->ToString()))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	/** @return The session's type. */
	FName GetSessionType() const
	{
		FString SessionType;
		if (OnlineResult.Session.SessionSettings.Get(SETTING_SESSIONTYPE, SessionType))
		{
			return FName(*SessionType);
		}

		return NAME_None;
	}

	/** @return The session's unique id. */
	const FUniqueNetId& GetSessionUniqueId() const
	{
		return OnlineResult.Session.SessionInfo->GetSessionId();
	}

	/** @return Session owner's unique id. */
	FUniqueNetIdRepl GetOwnerUniqueId() const
	{
		return OnlineResult.Session.OwningUserId;
	}

	/** @return Session owner's username. */
	FString GetOwnerUsername() const
	{
		return OnlineResult.Session.OwningUserName.Left(20);
	}

	/** @return Current number of players in the session. */
	int32 GetNumPlayers() const
	{
		return OnlineResult.Session.SessionSettings.NumPublicConnections - OnlineResult.Session.NumOpenPublicConnections;
	}

	/** @return The current session settings. */
	FKronosSessionSettings GetSessionSettings() const
	{
		return FKronosSessionSettings(OnlineResult.Session.SessionSettings);
	}

	/** Get the value of a specific session setting. The value is valid only if the function returns true. */
	template<typename ValueType>
	bool GetSessionSetting(const FName Key, ValueType& OutValue) const
	{
		return OnlineResult.Session.SessionSettings.Get(Key, OutValue);
	}
};

/**
 * Matchmaking flags.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "True"))
enum class EKronosMatchmakingFlags : uint8
{
	/** No flags. */
	None = 0x00 UMETA(Hidden),

	/** Matchmaking should never decide to host a session. */
	NoHost = 0x01,

	/** Matchmaking should skip reservation requesting when joining a session. */
	SkipReservation = 0x02,

	/** Matchmaking should skip skill rating checks. */
	SkipEloChecks = 0x04
};
ENUM_CLASS_FLAGS(EKronosMatchmakingFlags)

/**
 * Matchmaking async state flags.
 */
UENUM(meta = (Bitflags))
enum class EKronosMatchmakingAsyncStateFlags : uint8
{
	/** No flags. */
	None = 0x00,

	/** Waiting for session create request to complete. */
	CreatingSession = 0x01,

	/** Waiting for session join request to complete. */
	JoiningSession = 0x02,

	/** Waiting for reservation response. */
	RequestingReservation = 0x04,

	/** Waiting for search pass to cancel. */
	CancelingSearch = 0x08,

	/** Waiting for pending reservation to cancel. */
	CancelingReservationRequest = 0x10
};
ENUM_CLASS_FLAGS(EKronosMatchmakingAsyncStateFlags)

/**
 * Search pass async state flags.
 */
UENUM(meta = (Bitflags))
enum class EKronosSearchPassAsyncStateFlags : uint8
{
	/** No flags. */
	None = 0x00,

	/** Waiting for session search request to complete. */
	FindingSessions = 0x01,

	/** Waiting for session pinging to complete. */
	PingingSessions = 0x02,

	/** Waiting for session cancel search request to complete. */
	CancelingSearch = 0x04
};
ENUM_CLASS_FLAGS(EKronosSearchPassAsyncStateFlags)

/**
 * Possible matchmaking modes. These are used to define what the matchmaking is intended to do.
 *
 * These were introduced to reduce code complexity and size. Since core session management functionality (create/find/join) is part of the full matchmaking flow anyway,
 * the code itself can be reused. Only the matchmaking flow has to be a bit different for each mode.
 */
UENUM(BlueprintType)
enum class EKronosMatchmakingMode : uint8
{
	/** Full matchmaking. Attempts to find and join the best available session. Can switch over to hosting if no session is found. */
	Default,

	/** Create only matchmaking. Used when we only want to create a session. E.g. player wants to host a match. */
	CreateOnly,

	/** Search only matchmaking. Used when we only want to find sessions. E.g. a server browser. */
	SearchOnly,

	/** Join only matchmaking. Used when we only want to join a session. E.g. player wants to join a match from a server browser. */
	JoinOnly
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosMatchmakingMode Value)
{
	switch (Value)
	{
	case EKronosMatchmakingMode::Default:
		return TEXT("Default");
	case EKronosMatchmakingMode::CreateOnly:
		return TEXT("CreateOnly");
	case EKronosMatchmakingMode::SearchOnly:
		return TEXT("SearchOnly");
	case EKronosMatchmakingMode::JoinOnly:
		return TEXT("JoinOnly");
	default:
		return TEXT("");
	}
}

/**
 * Possible matchmaking states.
 */
UENUM(BlueprintType)
enum class EKronosMatchmakingState : uint8
{
	/** Matchmaking not started. */
	NotStarted,

	/** Matchmaking is starting. */
	Starting,

	/** Matchmaking is gathering sessions. */
	Searching,

	/** Matchmaking is requesting reservation with a session. */
	RequestingReservation,

	/** Matchmaking is joining a session. */
	JoiningSession,

	/** Matchmaking is creating a session. */
	CreatingSession,

	/** Matchmaking complete. */
	Complete,

	/** Matchmaking is being canceled. */
	Canceling,

	/** Matchmaking is canceled. */
	Canceled,

	/** Matchmaking failed internally. */
	Failure
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosMatchmakingState Value)
{
	switch (Value)
	{
	case EKronosMatchmakingState::NotStarted:
		return TEXT("NotStarted");
	case EKronosMatchmakingState::Starting:
		return TEXT("Starting");
	case EKronosMatchmakingState::Searching:
		return TEXT("Searching");
	case EKronosMatchmakingState::RequestingReservation:
		return TEXT("RequestingReservation");
	case EKronosMatchmakingState::JoiningSession:
		return TEXT("JoiningSession");
	case EKronosMatchmakingState::CreatingSession:
		return TEXT("CreatingSession");
	case EKronosMatchmakingState::Complete:
		return TEXT("Complete");
	case EKronosMatchmakingState::Canceling:
		return TEXT("Canceling");
	case EKronosMatchmakingState::Canceled:
		return TEXT("Canceled");
	case EKronosMatchmakingState::Failure:
		return TEXT("Failure");
	default:
		return TEXT("");
	}
}

/**
 * Possible matchmaking end results.
 */
UENUM(BlueprintType)
enum class EKronosMatchmakingCompleteResult : uint8
{
	/** Matchmaking complete with failure. */
	Failure,

	/** Matchmaking complete with no results. */
	NoResults,

	/** Matchmaking completed successfully. */
	Success,

	/** Matchmaking completed successfully by creating a new session. */
	SessionCreated,

	/** Matchmaking completed successfully by joining a session. */
	SessionJoined
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosMatchmakingCompleteResult Value)
{
	switch (Value)
	{
	case EKronosMatchmakingCompleteResult::Failure:
		return TEXT("Failure");
	case EKronosMatchmakingCompleteResult::NoResults:
		return TEXT("NoResults");
	case EKronosMatchmakingCompleteResult::Success:
		return TEXT("Success");
	case EKronosMatchmakingCompleteResult::SessionCreated:
		return TEXT("SessionCreated");
	case EKronosMatchmakingCompleteResult::SessionJoined:
		return TEXT("SessionJoined");
	default:
		return TEXT("");
	}
}

/**
 * Possible reasons behind a matchmaking failure.
 */
UENUM(BlueprintType)
enum class EKronosMatchmakingFailureReason : uint8
{
	/** Unknown failure. */
	Unknown,

	/** Invalid matchmaking params. */
	InvalidParams,

	/** Failure during a search pass. */
	SearchPassFailure,

	/** Failure while creating session. */
	CreateSessionFailure,

	/** Failure while joining session. */
	JoinSessionFailure
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosMatchmakingFailureReason Value)
{
	switch (Value)
	{
	case EKronosMatchmakingFailureReason::Unknown:
		return TEXT("Unknown");
	case EKronosMatchmakingFailureReason::InvalidParams:
		return TEXT("InvalidParams");
	case EKronosMatchmakingFailureReason::SearchPassFailure:
		return TEXT("SearchPassFailure");
	case EKronosMatchmakingFailureReason::CreateSessionFailure:
		return TEXT("CreateSessionFailure");
	case EKronosMatchmakingFailureReason::JoinSessionFailure:
		return TEXT("JoinSessionFailure");
	default:
		return TEXT("");
	}
}

/**
 * Possible search pass states.
 */
UENUM(BlueprintType)
enum class EKronosSearchPassState : uint8
{
	/** Search pass not started. */
	NotStarted,

	/** Search pass is gathering sessions. */
	Searching,

	/** Search pass is pinging sessions. */
	PingingSessions,

	/** Search pass is complete. */
	Complete,

	/** Search pass is being canceled. */
	Canceling,

	/** Search pass is canceled. */
	Canceled,

	/** Search pass failed internally. */
	Failure
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosSearchPassState Value)
{
	switch (Value)
	{
	case EKronosSearchPassState::NotStarted:
		return TEXT("NotStarted");
	case EKronosSearchPassState::Searching:
		return TEXT("Searching");
	case EKronosSearchPassState::PingingSessions:
		return TEXT("PingingSessions");
	case EKronosSearchPassState::Complete:
		return TEXT("Complete");
	case EKronosSearchPassState::Canceling:
		return TEXT("Canceling");
	case EKronosSearchPassState::Canceled:
		return TEXT("Canceled");
	case EKronosSearchPassState::Failure:
		return TEXT("Failure");
	default:
		return TEXT("");
	}
}

/**
 * Possible search pass end results.
 */
UENUM(BlueprintType)
enum class EKronosSearchPassCompleteResult : uint8
{
	/** Search pass complete with failure. */
	Failure,

	/** Search pass complete but no sessions were found. */
	NoSession,

	/** Search pass complete and found at least one session. */
	Success
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosSearchPassCompleteResult Value)
{
	switch (Value)
	{
	case EKronosSearchPassCompleteResult::Failure:
		return TEXT("Failure");
	case EKronosSearchPassCompleteResult::NoSession:
		return TEXT("NoSession");
	case EKronosSearchPassCompleteResult::Success:
		return TEXT("Success");
	default:
		return TEXT("");
	}
}

/**
 * A single member of a reservation.
 *
 * @see FKronosReservation
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosReservationMember
{
	GENERATED_BODY()

	/** UniqueId of the member. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Default")
	FUniqueNetIdRepl PlayerId;

	/** Whether the member has arrived at the session or not. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Default")
	bool bIsCompleted;

	/** Handle used to time-out the member. */
	FTimerHandle TimerHandle_ReservationTimeout;

	/** Default constructor. */
	FKronosReservationMember() :
		PlayerId(FUniqueNetIdRepl()),
		bIsCompleted(false),
		TimerHandle_ReservationTimeout(FTimerHandle())
	{}

	/** Preferred constructor. */
	FKronosReservationMember(const FUniqueNetIdRepl& InPlayerId) :
		PlayerId(InPlayerId),
		bIsCompleted(false),
		TimerHandle_ReservationTimeout(FTimerHandle())
	{}

	/** @return Whether the reservation member is valid or not. */
	bool IsValid() const
	{
		return PlayerId.IsValid();
	}
};

/**
 * A reservation with a session for a group of players.
 *
 * @see FKronosReservationMember
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosReservation
{
	GENERATED_BODY()

	/**
	 * UniqueId of the player who requested the reservation.
	 *
	 * NOTE: Internally this isn't used for anything at the moment, but it's here so that the system can be extended if needed.
	 * For example if the reservation owner leaves the session, we can tell all reservation members to leave as well and possibly reconnect to the party.
	 * The PreReservationOwnerRemoved event of the KronosReservationHost is a good place to handle this.
	 * If you wish to use this, cleanup needs to be done if the reservation owner leaves but other reservation members stay.
	 * 
	 * @see KronosReservationHost::PreReservationOwnerRemoved
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Default")
	FUniqueNetIdRepl ReservationOwner;

	/** List of reservation members, including the reservation owner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Default")
	TArray<FKronosReservationMember> ReservationMembers;

	/** @return Whether the reservation is valid or not. */
	bool IsValid(const bool bLogErrors = true) const
	{
		bool bIsValid = true;

		if (!ReservationOwner.IsValid())
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("ReservationOwner of FKronosReservation is invalid!"));
			bIsValid = false;
		}

		if (ReservationMembers.Num() == 0)
		{
			UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("ReservationMembers of FKronosReservation is invalid! The array is empty."));
			bIsValid = false;
		}

		for (const FKronosReservationMember& ResMember : ReservationMembers)
		{
			if (!ResMember.IsValid())
			{
				UE_CLOG(bLogErrors, LogKronos, Warning, TEXT("ReservationMembers of FKronosReservation is invalid! A reservation member is invalid."));
				bIsValid = false;
			}
		}

		return bIsValid;
	}
};

/**
 * Possible reservation request complete results.
 */
UENUM(BlueprintType)
enum class EKronosReservationCompleteResult : uint8
{
	/** Unknown error. */
	UnknownError,

	/** Reservation client lost connection, or failed to establish one with the reservation host. */
	ConnectionError,

	/** The requested reservation was invalid. */
	ReservationInvalid,

	/** Reservation limit reached. */
	ReservationLimitReached,

	/** A reservation member already have a reservation with the session. */
	ReservationDuplicate,

	/** Reservation host denying requests or a reservation member is banned from the session. */
	ReservationDenied,

	/** Reservation accepted. */
	ReservationAccepted
};

/** @return Converts enum into a character array that can easily be turned into strings. */
inline const TCHAR* LexToString(const EKronosReservationCompleteResult Value)
{
	switch (Value)
	{
	case EKronosReservationCompleteResult::UnknownError:
		return TEXT("UnknownError");
	case EKronosReservationCompleteResult::ConnectionError:
		return TEXT("ConnectionError");
	case EKronosReservationCompleteResult::ReservationInvalid:
		return TEXT("ReservationInvalid");
	case EKronosReservationCompleteResult::ReservationLimitReached:
		return TEXT("ReservationLimitReached");
	case EKronosReservationCompleteResult::ReservationDuplicate:
		return TEXT("ReservationDuplicate");
	case EKronosReservationCompleteResult::ReservationDenied:
		return TEXT("ReservationDenied");
	case EKronosReservationCompleteResult::ReservationAccepted:
		return TEXT("ReservationAccepted");
	default:
		return TEXT("");
	}
}

/**
 * Blueprint wrapper around the native FOnlineFriend class.
 * Exposes native online friend data to blueprints.
 */
USTRUCT(BlueprintType)
struct KRONOS_API FKronosOnlineFriend
{
	GENERATED_BODY()

	/** UniqueId of the user. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Default")
	FUniqueNetIdRepl UserId;

	/** Display name of the user. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Default")
	FString UserName;

	/** Whether the user is online or not. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Default")
	bool bIsOnline;

	/** Whether the user is playing this game. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Default")
	bool bIsInGame;

	/** Default constructor. */
	FKronosOnlineFriend();

	/** Constructor from a native online friend. */
	FKronosOnlineFriend(const FOnlineFriend& NativeOnlineFriend);

	/** @return Whether the friend data is valid or not. */
	bool IsValid() const
	{
		return UserId.IsValid();
	}
};
