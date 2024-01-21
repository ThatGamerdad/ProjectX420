// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Templates/SubclassOf.h"
#include "KronosConfig.generated.h"

/**
 * Config parameters of the Kronos Matchmaking plugin.
 */
UCLASS(Config = Engine, DefaultConfig)
class KRONOS_API UKronosConfig : public UObject
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UKronosConfig(const FObjectInitializer& ObjectInitializer);

	/** Get the global KronosConfig object. This returns the class's CDO, so every property is read only. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Get Kronos Config")
	static const UKronosConfig* Get();

public:

	/** Class to be used when the game instance is creating the online session object. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class UKronosOnlineSession> OnlineSessionClass;

	/** Class to be used when creating a new matchmaking policy. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class UKronosMatchmakingPolicy> MatchmakingPolicyClass;

	/** Class to be used when creating a new search pass. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class UKronosMatchmakingSearchPass> MatchmakingSearchPassClass;

	/** Class to be used when creating a party beacon host. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class AKronosPartyListener> PartyListenerClass;

	/** Class to be used when creating a party beacon host object. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class AKronosPartyHost> PartyHostClass;

	/** Class to be used when creating a party beacon client. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class AKronosPartyClient> PartyClientClass;

	/** Class to be used when creating the party beacon state. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class AKronosPartyState> PartyStateClass;

	/** Class to be used when creating a party beacon player state. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class AKronosPartyPlayerState> PartyPlayerStateClass;

	/** Class to be used when creating a reservation beacon host. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class AKronosReservationListener> ReservationListenerClass;

	/** Class to be used when creating a reservation beacon host object. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class AKronosReservationHost> ReservationHostClass;

	/** Class to be used when creating a reservation beacon client. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<class AKronosReservationClient> ReservationClientClass;

	/** Class to be used when creating the user manager of the online session. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes", AdvancedDisplay)
	TSubclassOf<class UKronosUserManager> UserManagerClass;

	/** Class to be used when creating the matchmaking manager of the online session. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes", AdvancedDisplay)
	TSubclassOf<class UKronosMatchmakingManager> MatchmakingManagerClass;

	/** Class to be used when creating the party manager of the online session. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes", AdvancedDisplay)
	TSubclassOf<class UKronosPartyManager> PartyManagerClass;

	/** Class to be used when creating the reservation manager of the online session. */
	UPROPERTY(Config, NoClear, EditAnywhere, BlueprintReadOnly, Category = "Classes", AdvancedDisplay)
	TSubclassOf<class UKronosReservationManager> ReservationManagerClass;

public:

	/**
	 * Whether the plugin should handle user authentication automatically.
	 * If enabled, user authentication will be started automatically when the game default map is loaded.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Authentication")
	bool bAuthenticateUserAutomatically;

	/**
	 * Minimum time to spend on each auth task during user authentication.
	 * This is useful if you want to display the current auth state to the player.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Authentication", meta = (ClampMin = "0.0"))
	float MinTimePerAuthTask;

	/**
	 * Delay in seconds before calling the enter game event after user authentication is complete.
	 * This is useful if you want to do UI animation before entering the game's main menu (e.g. screen fade to black) after authentication.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Authentication", meta = (ClampMin = "0.0"))
	float EnterGameDelayAfterAuth;

	/**
	 * Overrides which map is considered to be the game default map.
	 * If empty, the game default map will be the one that is set in the project setting.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Authentication", AdvancedDisplay, meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath GameDefaultMapOverride;

public:

	/**
	 * Whether the online subsystem of choice supports the IOnlineSession::FindFriendSession function.
	 * Steam supports this, EOS does not.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	bool bFindFriendSessionSupported;

	/**
	 * Whether the online subsystem of choice supports the IOnlineSession::FindSessionById function.
	 * EOS supports this, Steam does not.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	bool bFindSessionByIdSupported;

	/** Delay in seconds before starting a new matchmaking pass. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Matchmaking", meta = (ClampMin = "0.0"))
	float RestartMatchmakingPassDelay;

	/** Delay in seconds before starting a new search pass. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Matchmaking", meta = (ClampMin = "0.0"))
	float RestartSearchPassDelay;

	/** Amount of time to wait for search results when doing a regular search. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Matchmaking", meta = (ClampMin = "0.0"))
	float SearchTimeout;

public:

	/**
	 * Delay in seconds before party clients start to follow the party leader to a session.
	 * This is especially important when the party host is also going to be the host of the session that the party is joining.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Party", meta = (ClampMin = "0.0"))
	float ClientFollowPartyToSessionDelay;

	/** Amount of search attempts to make for the party leader's session. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Party", meta = (ClampMin = "1"))
	int32 ClientFollowPartyAttempts;

	/** Delay in seconds before party clients start to search for the party when reconnecting. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Party", meta = (ClampMin = "0.0"))
	float ClientReconnectPartyDelay;

	/** Amount of search attempts to make for the party when reconnecting. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Party", meta = (ClampMin = "1"))
	int32 ClientReconnectPartyAttempts;

public:

	/** Delay in seconds before the session host travels to the session for the first time. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Traveling", meta = (ClampMin = "0.0"))
	float ServerTravelToSessionDelay;

	/** Delay in seconds before attempting to resolve connection with a session and traveling to it. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Traveling", meta = (ClampMin = "0.0"))
	float ClientTravelToSessionDelay;

public:

	/** Delay in seconds before removing an incomplete reservation (e.g. player hasn't arrived at the session). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Reservation", meta = (ClampMin = "0.0"))
	float ReservationTimeout;
};
