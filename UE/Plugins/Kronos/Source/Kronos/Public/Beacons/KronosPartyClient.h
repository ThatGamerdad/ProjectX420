// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LobbyBeaconClient.h"
#include "KronosTypes.h"
#include "KronosPartyClient.generated.h"

class AKronosPartyState;
class AKronosPartyPlayerState;

/**
 * A beacon client which represents a player in a party. Exists on both the server and the client.
 * Similar to the player controller in the Unreal networking architecture.
 */
UCLASS()
class KRONOS_API AKronosPartyClient : public ALobbyBeaconClient
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	AKronosPartyClient(const FObjectInitializer& ObjectInitializer);

protected:

	/** Handle used when waiting for initial replication to finish. */
	FTimerHandle TimerHandle_WaitingInitialReplication;

	/** Parameters to be used when following the party to a session. */
	FKronosFollowPartyParams FollowPartyParams;

public:

	/**
	 * Initialize the player server side. RPC replication is ready.
	 * If you have a backend service this is a good place to read the player's skill rating, fetch cosmetics, etc...
	 */
	virtual void ServerInitPlayer();

	/** Initialize the player client side. RPC replication is ready. */
	virtual void ClientInitPlayer();

	/**
	 * Changes the elo score representing the player's skill level.
	 * Can be called from clients, but in an ideal world the server would fetch this value from a backend service.
	 */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void SetPlayerElo(int32 NewPlayerElo);

	/** Request a player elo change with the server. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSetPlayerElo(const int32 NewPlayerElo);

	/** Changes player data. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void SetPlayerData(const TArray<int32>& NewPlayerData);

	/** Request a player data change with the server. */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSetPlayerData(const TArray<int32>& NewPlayerData);

	/** Send a chat message to all party members. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void SendChatMessage(const FString& Msg);

	/** Tell the server to send a chat message to all party members. */
	UFUNCTION(Server, Reliable)
	virtual void ServerSendChatMessage(const FString& Msg);

	/** Replicate the chat message to the client. */
	UFUNCTION(Client, Reliable)
	virtual void ClientReceiveChatMessage(const FUniqueNetIdRepl& SenderId, const FString& Msg);

	/** Tell the client to start following the party to the session. */
	UFUNCTION(Client, Reliable)
	virtual void ClientFollowPartyToGameSession(const FKronosFollowPartyParams& FollowParams);

	/**
	 * Determine whether the initial replication has finished for the client.
	 * If true, client login will be finished.
	 * Called periodically after ClientLoginComplete_Implementation() is called.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Default", DisplayName = "Get Initial Replication Props")
	bool GetInitialReplicationProps() const;

	/** Get the party state. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual AKronosPartyState* GetPartyState() const;

	/** Get the client's party player state. */
	UFUNCTION(BlueprintPure, Category = "Default", DisplayName = "Get Player State")
	virtual AKronosPartyPlayerState* GetPartyPlayerState() const;

	/** Get the client's elo score from his party player state. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetPlayerElo() const;

	/** Get the client's player data from his party player state. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual TArray<int32> GetPlayerData() const;

	/** Check whether this player is the local player or not. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual bool IsLocalPlayer() const;

protected:

	/** Called when the client has connected to the party. */
	virtual void OnPartyLoginComplete(bool bWasSuccessful);

	/** Called when a player has joined to the party. */
	virtual void OnPartyPlayerJoined(const FText& DisplayName, const FUniqueNetIdRepl& UniqueId);

	/** Called when a player has left the party. */
	virtual void OnPartyPlayerLeft(const FUniqueNetIdRepl& UniqueId);

	/** Called when the party session's settings have been updated. */
	UFUNCTION()
	virtual void OnPartyUpdated(bool bWasSuccessful);

	/** Called when the client is told by the server to follow the party to a session. */
	virtual void HandleJoiningGame();

	/** Called when the server acknowledges that the client is following the party to the session. */
	virtual void HandleJoiningGameAck();

protected:

	/** Event when the client is connecting to a party host. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Connecting To Party")
	void K2_OnConnectingToParty();

	/** Event when the client has lost connection with the party host or failed to establish one. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Connection Failure")
	void K2_OnConnectionFailure();

	/** Event when the client has connected to the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Party Login Complete")
	void K2_OnPartyLoginComplete(bool bWasSuccessful);

	/**
	 * Event when initializing the player server side. RPC replication is ready.
	 * If you have a backend service this is a good place to read the player's skill rating, fetch cosmetics, etc...
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "Server Init Player")
	void K2_ServerInitPlayer();

	/** Event when initializing the player client side. RPC replication is ready. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "Client Init Player")
	void K2_ClientInitPlayer();

	/** Event when a player has joined to the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Joined Party")
	void K2_OnPartyPlayerJoined(const FText& DisplayName, const FUniqueNetIdRepl& UniqueId);

	/** Event when a player has left the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Left Party")
	void K2_OnPartyPlayerLeft(const FUniqueNetIdRepl& UniqueId);

	/** Event when the client is about to follow the party to a match. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Following Party To Game")
	void K2_HandleJoiningGameAck();

	/** Event when the client was kicked from the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Client Was Kicked")
	void K2_ClientWasKicked(const FText& KickReason);

protected:

	/** Called by the party manager when the client is initialized and connecting to a party. */
	virtual void ClientConnectingToParty();

	/** Assign the proper session id to the client. */
	virtual void SetDestSessionId(const FString& InSessionId) { DestSessionId = InSessionId; }

	friend class UKronosPartyManager;

public:

	//~ Begin ALobbyBeaconClient Interface
	virtual void ClientLoginComplete_Implementation(const FUniqueNetIdRepl& InUniqueId, bool bWasSuccessful) override;
	virtual void ClientWasKicked_Implementation(const FText& KickReason) override;
	virtual void OnFailure() override;
	virtual void DestroyBeacon() override;
	//~ End ALobbyBeaconClient Interface

public:

	/** @return Party debug data to be displayed in the Gameplay Debugger. */
	virtual FString GetDebugString() const;
};
