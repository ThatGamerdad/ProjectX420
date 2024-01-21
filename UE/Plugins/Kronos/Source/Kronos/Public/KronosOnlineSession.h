// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/OnlineSession.h"
#include "KronosTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "KronosOnlineSession.generated.h"

class UKronosUserManager;
class UKronosMatchmakingManager;
class UKronosPartyManager;
class UKronosReservationManager;
class AGameModeBase;

/**
 * Delegate triggered when a game session's settings have been updated.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdateKronosMatchComplete, bool, bWasSuccessful);

/**
 * Delegate triggered when a party session's settings have been updated.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdateKronosPartyComplete, bool, bWasSuccessful);

/**
 * KronosOnlineSession is the primary manager of online services.
 * It hosts multiple smaller online managers that each handle one aspect of the online service such as the online user, matchmaking, parties, and more.
 * It is responsible for hosting and/or connecting players to online matches after they successfully joined the session on the backend.
 * This class also implements online session related functions, and handles session cleanup procedures.
 *
 * OnlineSession classes in Unreal Engine are automatically spawned and managed by the GameInstance class.
 * You can safely store 'global' variables in these classes that should persist through map changes.
 *
 * To minimize dependencies, some functions (e.g. traveling to session) that are better suited for the GameInstance class are implemented here instead.
 * For more information, check out the KronosGameInstance class.
 *
 * @see UKronosGameInstance
 */
UCLASS(Blueprintable, BlueprintType)
class KRONOS_API UKronosOnlineSession : public UOnlineSession
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UKronosOnlineSession(const FObjectInitializer& ObjectInitializer);

protected:

	/** The user manager of the online session. */
	UPROPERTY(Transient)
	UKronosUserManager* UserManager;

	/** The matchmaking manager of the online session. */
	UPROPERTY(Transient)
	UKronosMatchmakingManager* MatchmakingManager;

	/** The party manager of the online session. */
	UPROPERTY(Transient)
	UKronosPartyManager* PartyManager;

	/** The reservation manager of the online session. */
	UPROPERTY(Transient)
	UKronosReservationManager* ReservationManager;

protected:

	/** Whether we are currently cleaning up a session after a disconnect. */
	bool bHandlingCleanup;

	/** Delegate triggered when a session is updated. */
	FOnUpdateSessionCompleteDelegate OnUpdateSessionCompleteDelegate;

	/** Delegate triggered when a session is cleaned up after a disconnect. */
	FOnDestroySessionCompleteDelegate OnCleanupSessionCompleteDelegate;

	/** Delegate triggered when the user accepts a session invitation. */
	FOnSessionUserInviteAcceptedDelegate OnSessionUserInviteAcceptedDelegate;

	/** Handle for game mode initialized delegate. */
	FDelegateHandle GameModeInitializedDelegateHandle;

	/** Handle for update session complete delegate. */
	FDelegateHandle OnUpdateSessionCompleteDelegateHandle;

	/** Handle for cleanup session completion delegate. */
	FDelegateHandle OnCleanupSessionCompleteDelegateHandle;

	/** Handle for session invitation accepted delegate. */
	FDelegateHandle OnSessionUserInviteAcceptedDelegateHandle;

	/** Handle used to delay the enter game event after user auth is complete. */
	FTimerHandle TimerHandle_EnterGame;

	/** Handle used to delay travel to session calls. */
	FTimerHandle TimerHandle_TravelToSession;

private:

	/** Event when a game session's settings have been updated. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnUpdateKronosMatchComplete OnUpdatedMatchCompleteEvent;

	/** Event when a party session's settings have been updated. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnUpdateKronosPartyComplete OnUpdatePartyCompleteEvent;

public:

	/** Get the global KronosOnlineSession from the game instance. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Get Kronos Online Session", meta = (WorldContext = "WorldContextObject"))
	static UKronosOnlineSession* Get(const UObject* WorldContextObject);

	/** @return The user manager of the online session. */
	UKronosUserManager* GetUserManager() const { return UserManager; }

	/** @return The matchmaking manager of the online session. */
	UKronosMatchmakingManager* GetMatchmakingManager() const { return MatchmakingManager; }

	/** @return The party manager of the online session. */
	UKronosPartyManager* GetPartyManager() const { return PartyManager; }

	/** @return The reservation manager of the online session. */
	UKronosReservationManager* GetReservationManager() const { return ReservationManager; }

	/** @return The delegate fired when updating a game session is complete. */
	FOnUpdateKronosMatchComplete& OnUpdateMatchComplete() { return OnUpdatedMatchCompleteEvent; }

	/** @return The delegate fired when updating a party session is complete. */
	FOnUpdateKronosPartyComplete& OnUpdatePartyComplete() { return OnUpdatePartyCompleteEvent; }

protected:

	/** Entry point when the game mode has been initialized for the recently loaded map. */
	void OnGameModeInitialized(AGameModeBase* GameMode);

	/**
	 * Called when the recently loaded map is the game default map.
	 * For most games this is going to be the game's main menu.
	 * The game mode of the map has already been initialized at this point.
	 */
	virtual void OnGameDefaultMapLoaded();

public:

	/** Entry point after user authentication is complete. Automatically called by the user manager. */
	virtual void HandleUserAuthComplete(EKronosUserAuthCompleteResult Result, bool bWasInitialAuth, const FText& ErrorText);

	/**
	 * Called when the user is ready to enter the game default map after authentication.
	 * This is basically the entry point to the game's main menu.
	 * You can create the main menu widget here, handle party reconnection, add popups to the screen, etc...
	 * 
	 * @param bIsInitialLogin Whether it is the user's first time entering the game default map.
	 */
	virtual void OnEnterGame(bool bIsInitialLogin);

public:

	/** Entry point after matchmaking is complete. Automatically called by the matchmaking policy object. */
	virtual void HandleMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result);

	/** Handles matchmaking complete with session created event. At this point the session has been created, and the player should begin hosting the match. */
	virtual void HandleCreatingSession(const FName InSessionName);

	/** Handles matchmaking complete with session joined event. At this point the session has been joined, and the player should begin connecting to the host. */
	virtual void HandleJoiningSession(const FName InSessionName);

	/** Begins connecting the party to the session before traveling. */
	virtual bool ConnectPartyToGameSession();

	/** Called for clients when the party leader signals that the party is joining a session. */
	virtual bool FollowPartyToGameSession(const FKronosFollowPartyParams& FollowPartyParams);

	/** Called before we start matchmaking for the session our party is joining. */
	virtual void OnFollowPartyToSessionStarted();

	/** Called when the matchmaking for the session our party is joining is complete. */
	virtual void OnFollowPartyToSessionComplete(const FName SessionName, const EKronosMatchmakingCompleteResult Result);

	/** Called when there was a failure following the party. */
	virtual void OnFollowPartyToSessionFailure();

	/** Called after a new session was created. Hosts a match for the session. */
	virtual void ServerTravelToGameSession();

	/** Called after joining a session. Begins traveling to the match advertised by the session. */
	virtual void ClientTravelToGameSession();

protected:

	/** Called when a session have been updated. */
	virtual void OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful);

	/**
	 * Called before accepting an invite to figure out if we are in a good state.
	 * 
	 * @param Session The session that we are accepting the invite for.
	 * @param bIsPartyInvite Whether this is an invite for a party session or a game session (match).
	 * 
	 * @return If true, the user will join the session.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Default", DisplayName = "Can Accept Session Invite")
	bool CanAcceptSessionInvite(const FKronosSearchResult& Session, bool bIsPartyInvite) const;

	/** Called when a match invite is accepted. */
	virtual void OnGameSessionInviteAccepted(const FKronosSearchResult& Session);

	/** Called when a party invite is accepted. */
	virtual void OnPartySessionInviteAccepted(const FKronosSearchResult& Session);

	/** Cleans up the session after a disconnect. Called during HandleDisconnect(). */
	virtual void CleanupSession(UWorld* World, UNetDriver* NetDriver);

	/** Called when the session is cleaned up after a disconnect. */
	virtual void OnCleanupSessionComplete(FName SessionName, bool bWasSuccessful);

protected:

	/**
	 * Event when the game default map is loaded.
	 * For most games this is going to be the game's main menu.
	 * The game mode of the map has already been initialized at this point.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Game Default Map Loaded")
	void K2_OnGameDefaultMapLoaded();

	/**
	 * Event when the user is ready to enter the game default map after authentication.
	 * This is basically the entry point to the game's main menu.
	 * You can create the main menu widget here, handle party reconnection, add popups to the screen, etc... 
	 * 
	 * @param bIsInitialLogin Whether it is the user's first time entering the game default map.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Enter Game")
	void K2_OnEnterGame(bool bIsInitialLogin);

	/** Event before we start matchmaking for the session our party is joining. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Client Follow Party Started")
	void K2_OnFollowPartyToSessionStarted();

	/**
	 * Event when the matchmaking for the session our party is joining is complete.
	 * 
	 * @param SessionName The type of the session. Either 'GameSession' or 'PartySession'.
	 * @param Result The result of matchmaking for the party host's session when following the party.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Client Follow Party Complete")
	void K2_OnFollowPartyToSessionComplete(const FName SessionName, const EKronosMatchmakingCompleteResult Result);

	/** Event when there was a failure following the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Client Follow Party Failure")
	void K2_OnFollowPartyToSessionFailure();

	/**
	 * Event when a match invite is accepted.
	 * 
	 * @param Session The session that we are accepting the invite for.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Match Invite Accepted")
	void K2_OnGameSessionInviteAccepted(const FKronosSearchResult& Session);

	/**
	 * Event when a party invite is accepted.
	 * 
	 * @param Session The session that we are accepting the invite for.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Party Invite Accepted")
	void K2_OnPartySessionInviteAccepted(const FKronosSearchResult& Session);

	/** Event when the session is being cleaned up after a disconnect. Only called for game sessions. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Default", DisplayName = "On Cleanup Session")
	void K2_OnCleanupSession();

public:

	/** Get the current state of a session. Can be used to check if a session exists or not. */
	virtual EOnlineSessionState::Type GetSessionState(const FName SessionName) const;

	/** Get the current configuration of an existing session. */
	virtual bool GetSessionSettings(const FName SessionName, FKronosSessionSettings& OutSessionSettings);

	/** Get a specific session setting from an existing session. */
	template<typename ValueType>
	bool GetSessionSetting(const FName SessionName, const FName Key, ValueType& OutValue);

	/**
	 * Updates the configuration of an existing session.
	 * NOTE: This operation is async.
	 *
	 * Do NOT attempt to update a session while it is brand new (created / joined in the current frame).
	 * Otherwise you may experience weird issues. (E.g. the session cannot be found by others).
	 *
	 * @param SessionName The name of the session to update.
	 * @param InSessionSettings The updated session settings.
	 * @param bShouldRefreshOnlineData Whether to submit the data to the backend or not.
	 * @param InExtraSessionSettings List of session settings to update on the session.
	 *
	 * @return True if the update was requested successfully, false otherwise.
	 */
	virtual bool UpdateSession(const FName SessionName, const FKronosSessionSettings& InSessionSettings, const bool bShouldRefreshOnlineData = true, const TArray<FKronosSessionSetting>& InExtraSessionSettings = TArray<FKronosSessionSetting>());

	/** Register the player as being part of the session. */
	virtual bool RegisterPlayer(const FName SessionName, const FUniqueNetIdRepl& PlayerId, const bool bWasFromInvite);

	/** Register the given players as being part of the session. */
	virtual bool RegisterPlayers(const FName SessionName, const TArray<FUniqueNetIdRepl>& PlayerIds);

	/** Unregister the player from the session. */
	virtual bool UnregisterPlayer(const FName SessionName, const FUniqueNetIdRepl& PlayerId);

	/** Unregister the given players from the session. */
	virtual bool UnregisterPlayers(const FName SessionName, const TArray<FUniqueNetIdRepl>& PlayerIds);

	/** Destroy the given session. */
	virtual bool DestroySession(const FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate());

	/**
	 * Ban the given player from an existing session.
	 * NOTE: This operation is async.
	 *
	 * @param SessionName The name of the session to ban the player from.
	 * @param PlayerId Unique id of the player we want to ban.
	 *
	 * @return True if the ban was requested successfully, false otherwise.
	 */
	virtual bool BanPlayerFromSession(const FName SessionName, const FUniqueNetId& PlayerId);

	/** Check if the given player is banned from the session. */
	virtual bool IsPlayerBannedFromSession(const FName SessionName, const FUniqueNetId& PlayerId);

public:

	//~ Begin UObject interface
	virtual UWorld* GetWorld() const final; // Required by blueprints to have access to gameplay statics and other world context based nodes.
	//~ End UObject interface

	//~ Begin UOnlineSession interface
	virtual void RegisterOnlineDelegates() override;
	virtual void ClearOnlineDelegates() override;
	virtual void HandleDisconnect(UWorld* World, class UNetDriver* NetDriver) override;
	virtual void StartOnlineSession(FName SessionName) override;
	virtual void EndOnlineSession(FName SessionName) override;
	virtual void OnSessionUserInviteAccepted(const bool bWasSuccess, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult) override;
	//~ End UOnlineSession interface

public:

	/** @return Session debug data to be displayed in the Gameplay Debugger. */
	virtual FString GetSessionDebugString(FName SessionName) const;
};
