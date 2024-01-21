// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LobbyBeaconHost.h"
#include "KronosTypes.h"
#include "KronosPartyHost.generated.h"

/** Time delay between checking whether all players have acknowledged the follow party request or not. */
#define CONNECT_PARTY_TO_GAMESESSION_TICKRATE 0.1f
/** Time before the party leader ignores all remaining players who haven't acknowledged the follow party request, and starts traveling. */
#define CONNECT_PARTY_TO_GAMESESSION_TIMEOUT 10.0f

class UKronosPartyManager;
class AKronosPartyClient;
class AKronosPartyState;

/**
 * A beacon host object that handles party client connections. Exists only on the server!
 * Similar to the game mode in the Unreal networking architecture.
 */
UCLASS()
class KRONOS_API AKronosPartyHost : public ALobbyBeaconHost
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	AKronosPartyHost(const FObjectInitializer& ObjectInitializer);

protected:

	/** Handle used when connecting the party to a session. */
	FTimerHandle TimerHandle_ConnectingPartyToGameSession;

	/** Handle used to timeout the attempt of connecting the party to a session. */
	FTimerHandle TimerHandle_TimeoutConnectingPartyToGameSession;

public:

	/** Handles a player elo change request. */
	virtual void ProcessPlayerEloChange(AKronosPartyClient* ClientActor, int32 NewPlayerElo);

	/** Handles a player data change request for the given client. */
	virtual void ProcessPlayerDataChange(AKronosPartyClient* ClientActor, const TArray<int32>& NewPlayerData);

	/** Handles the broadcasting of a chat message. */
	virtual void ProcessChatMessage(const FUniqueNetIdRepl& SenderId, const FString& Msg);

	/** Handles party leader started/stopped matchmaking. */
	virtual void ProcessPartyLeaderMatchmaking(bool bMatchmaking);

	/** Handles connecting the party to the same session. Called by the KronosOnlineSession when the party leader has created or joined a game. */
	virtual bool ProcessConnectPartyToGameSession();

	/** Create follow party params for the given client. The client will use this to find and join the desired session. */
	virtual FKronosFollowPartyParams MakeFollowPartyParamsForClient(class AKronosPartyPlayerState* InClient);

	/** Get the party state. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual AKronosPartyState* GetPartyState() const;

protected:

	/** Called when this host beacon is initialized by the party manager. */
	virtual void OnInitialized();

	/** Called continuously while connecting the party to a session. Checks if all clients have started following the party or not. */
	virtual void TickConnectingPartyToGameSession();

	/** Called when all clients have started following the party to the session. */
	virtual void OnConnectPartyToGameSessionComplete();

	/** Called when connecting the party to a session times out. */
	virtual void OnConnectPartyToGameSessionTimeout();

	/** Called when connecting the party to the session has completed. */
	virtual void TravelToGameSession();

	friend UKronosPartyManager;

protected:

	/** Event when this host beacon is initialized by the party manager. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Initialized")
	void K2_OnInitialized();

	/**
	 * Event when a new client is joining the party.
	 * Called after the client has connected but before the login process is started.
	 * This is useful when you have to react to party player change immediately (e.g. to cancel matchmaking).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Client Joining Party")
	void K2_OnClientJoiningParty();

	/**
	 * Event when a new client has joined the party.
	 * Called after the client is connected and logged in.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Client Joined Party")
	void K2_OnClientJoinedParty(AKronosPartyClient* PartyClient);

	/**
	 * Event when a client is leaving the party.
	 * Called before the client is disconnected.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Client Leaving Party")
	void K2_OnClientLeavingParty(AKronosPartyClient* PartyClient);

public:

	//~ Begin ALobbyBeaconHost Interface
	virtual void OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection) override;
	virtual bool PreLogin(const FUniqueNetIdRepl& InUniqueId, const FString& Options) override;
	virtual ALobbyBeaconPlayerState* HandlePlayerLogin(ALobbyBeaconClient* ClientActor, const FUniqueNetIdRepl& InUniqueId, const FString& Options) override;
	virtual void PostLogin(ALobbyBeaconClient* ClientActor) override;
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	//~ End ALobbyBeaconHost Interface
};
