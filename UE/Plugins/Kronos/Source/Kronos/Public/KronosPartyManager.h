// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "KronosTypes.h"
#include "KronosPartyManager.generated.h"

class AKronosPartyListener;
class AKronosPartyHost;
class AKronosPartyClient;
class AKronosPartyState;
class AKronosPartyPlayerState;

/**
 * Delegate triggered when we joined a party.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnectedToKronosParty);

/**
 * Delegate triggered when we left a party.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisconnectedFromKronosParty);

/**
 * Delegate triggered when we were kicked from a party.
 *
 * @param KickReason The reason for the kick.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKickedFromKronosParty, const FText&, KickReason);

/**
 * Delegate triggered when a new player joins the party.
 *
 * @param DisplayName Name of the player joining the party.
 * @param PlayerId UniqueId of the player joining the party.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerJoinedKronosParty, const FText&, DisplayName, const FUniqueNetIdRepl&, PlayerId);

/**
 * Delegate triggered when an existing player leaves the party.
 *
 * @param PlayerId UniqueId of the player leaving the party.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeftKronosParty, const FUniqueNetIdRepl&, PlayerId);

/**
 * Delegate triggered when a player state is being changed in the party (added / removed).
 *
 * @param PlayerState The player of interest (post-addition / pre-removal).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosPartyPlayerStateChanged, AKronosPartyPlayerState*, PlayerState);

/**
 * Delegate triggered when a party chat message is received.
 *
 * @param SenderId UniqueId of the player sending the message.
 * @param Msg The message that was received.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKronosPartyChatMessageReceived, const FUniqueNetIdRepl&, SenderId, const FString&, Msg);

/**
 * Delegate triggered when the party leader starts/stops matchmaking.
 *
 * @param bMatchmaking Whether the party leader is matchmaking or not.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosPartyLeaderMatchmaking, bool, bMatchmaking);

/**
 * Delegate triggered when we are following the party to a session. Only called for party clients.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKronosFollowPartyStarted);

/**
 * Delegate triggered when there was an error following the party to a session.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKronosFollowPartyFailure);

/**
 * KronosPartyManager is responsible for handling the online party beacons of the user.
 * It is automatically spawned and managed by the KronosOnlineSession class.
 * 
 * Online Beacons are used to establish a lightweight connection between players without a normal game connection.
 * They allow us to connect to a server without traveling to it first. In this case, the party host.
 * 
 * @see AKronosPartyListener
 * @see AKronosPartyHost
 * @see AKronosPartyClient
 */
UCLASS(Blueprintable, BlueprintType)
class KRONOS_API UKronosPartyManager : public UObject
{
	GENERATED_BODY()

protected:

	/** Party listener beacon. Only valid while we are hosting a party. */
	UPROPERTY(Transient)
	AKronosPartyListener* PartyBeaconListener;

	/** Party host beacon. Only valid while we are hosting a party. */
	UPROPERTY(Transient)
	AKronosPartyHost* PartyBeaconHost;

	/** Party client beacon. Only valid while we are in a party. */
	UPROPERTY(Transient)
	AKronosPartyClient* PartyBeaconClient;

	/**
	 * Cached information about the last party that we were a part of.
	 * Updated automatically while in a party.
	 */
	FKronosLastPartyInfo LastPartyInfo;

private:

	/** Event when we joined a party. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnConnectedToKronosParty OnConnectedToPartyEvent;

	/** Event when we left a party. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnDisconnectedFromKronosParty OnDisconnectedFromPartyEvent;

	/** Event when we were kicked from a party. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKickedFromKronosParty OnKickedFromPartyEvent;

	/** Event when a new player joins the party. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnPlayerJoinedKronosParty OnPlayerJoinedPartyEvent;

	/** Event when an existing player leaves the party. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnPlayerLeftKronosParty OnPlayerLeftPartyEvent;

	/** Event when a player state has been added to the party state. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosPartyPlayerStateChanged OnPlayerStateAddedEvent;

	/** Event when a player state is going to be removed from the party state. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosPartyPlayerStateChanged OnPlayerStateRemovedEvent;

	/** Event when a party chat message is received. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosPartyChatMessageReceived OnChatMessageReceivedEvent;

	/** Event when the party leader starts/stops matchmaking. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosPartyLeaderMatchmaking OnPartyLeaderMatchmakingEvent;

	/** Event when we are following the party to a session. Only called for party clients. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosFollowPartyStarted OnFollowingPartyToSessionEvent;

	/** Event when there was a failure following the party to a session. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosFollowPartyFailure OnFollowPartyFailureEvent;

public:

	/** Get the party manager from the KronosOnlineSession. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Get Kronos Party Manager", meta = (WorldContext = "WorldContextObject"))
	static UKronosPartyManager* Get(const UObject* WorldContextObject);

	/**
	 * Initialize the KronosPartyManager during game startup.
	 * Called by the KronosOnlineSession.
	 */
	virtual void Initialize() {};

	/**
	 * Deinitialize the KronosPartyManager before game shutdown.
	 * Called by the KronosOnlineSession.
	 */
	virtual void Deinitialize() {};

public:

	/**
	 * Initializes a party host beacon. Clients will connect to this beacon after they join the party session.
	 * Should be called after a party session has been created.
	 */
	virtual bool InitPartyBeaconHost(const int32 MaxNumPlayers);

	/**
	 * Initializes a party client beacon for the party host.
	 * Called during InitPartyBeaconHost().
	 */
	virtual bool InitPartyBeaconClientForHost();

	/**
	 * Initializes a party client beacon that will connect to the party session owner's host beacon.
	 * Should be called after the player successfully joined a party session.
	 */
	virtual bool InitPartyBeaconClient();

	/**
	 * Leave the party with no intention of reconnecting later.
	 * Should only be called for user requested disconnects.
	 * If the user has to leave the party due to game control flow, use LeavePartyInternal() instead.
	 */
	virtual void LeaveParty(const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate());

	/** Handle leaving party. Begins destroying all party beacons and leaves the party session. */
	virtual void LeavePartyInternal(const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate());

	/** Destroys all party beacons. This means that we are leaving/disbanding the party. */
	virtual void DestroyPartyBeacons();

	/** Kicks the given player from the party. */
	virtual void KickPlayerFromParty(const FUniqueNetIdRepl& UniqueId, const FText& Reason, bool bBanPlayerFromSession = false);

	/** Set whether the party leader is matchmaking or not. Should only be called from the server (party host). */
	virtual void SetPartyLeaderMatchmaking(bool bMatchmaking);

	/** Clear last party information. Indicates that the user has left the party intentionally. */
	virtual void ClearLastPartyInfo() { LastPartyInfo = FKronosLastPartyInfo(); };

	/**
	 * Update the last party info to reflect the new party that we've just joined or created.
	 * Called automatically by the KronosPartyClient while in a party.
	 */
	virtual void UpdateLastPartyInfo();

	/** @return Whether we are in a party or not. */
	virtual bool IsInParty() const;

	/** @return Whether we are a party leader or not. */
	virtual bool IsPartyLeader() const;

	/** @return Whether all connected clients are logged in to the party. */
	virtual bool IsEveryClientInParty() const;

	/** @return UniqueId of the party leader. */
	virtual bool GetPartyLeaderUniqueId(FUniqueNetIdRepl& OutUniqueId) const;

	/** @return Whether the party leader is currently matchmaking or not. */
	virtual bool IsPartyLeaderMatchmaking() const;

	/** @return Number of players in the party, or 1 if not in a party. */
	virtual int32 GetPartySize() const;

	/** @return Number of players in the party. */
	virtual int32 GetNumPlayersInParty() const;

	/** @return Max number of players in the party. */
	virtual int32 GetMaxNumPlayersInParty() const;

	/** @return The average elo score of the party. */
	virtual int32 GetPartyEloAverage() const;

	/** @return Current party state. */
	virtual AKronosPartyState* GetPartyState() const;

	/** @return Client actor of the given party player. */
	virtual AKronosPartyClient* GetPartyClient(const FUniqueNetIdRepl& UniqueId);

	/** @return Player state of the given party player. */
	virtual AKronosPartyPlayerState* GetPartyPlayerState(const FUniqueNetIdRepl& UniqueId);

	/** @return Player states of all party players. */
	virtual TArray<AKronosPartyPlayerState*> GetPartyPlayerStates() const;

	/** @return UniqueId of all party players. */
	virtual TArray<FUniqueNetIdRepl> GetPartyPlayerUniqueIds() const;

	/** @return Info about the last party that we were a part of. */
	const FKronosLastPartyInfo& GetLastPartyInfo() const { return LastPartyInfo; }

	/** @return The party listener beacon. */
	AKronosPartyListener* GetListenerBeacon() const { return PartyBeaconListener; }

	/** @return The party host beacon. */
	AKronosPartyHost* GetHostBeacon() const { return PartyBeaconHost; }

	/** @return The party client beacon. */
	AKronosPartyClient* GetClientBeacon() const { return PartyBeaconClient; }

	/** @return The delegate fired when we joined a party. */
	FOnConnectedToKronosParty& OnConnectedToParty() { return OnConnectedToPartyEvent; }

	/** @return The delegate fired when we left a party. */
	FOnDisconnectedFromKronosParty& OnDisconnectedFromParty() { return OnDisconnectedFromPartyEvent; }

	/** @return The delegate fired when we were kicked from a party. */
	FOnKickedFromKronosParty& OnKickedFromParty() { return OnKickedFromPartyEvent; }

	/** @return The delegate fired when a new player joins the party. */
	FOnPlayerJoinedKronosParty& OnPlayerJoinedParty() { return OnPlayerJoinedPartyEvent; }

	/** @return The delegate fired when an existing player leaves the party. */
	FOnPlayerLeftKronosParty& OnPlayerLeftParty() { return OnPlayerLeftPartyEvent; }

	/** @return The delegate fired when a player state has been added to the party state. */
	FOnKronosPartyPlayerStateChanged& OnPlayerStateAdded() { return OnPlayerStateAddedEvent; }

	/** @return The delegate fired when a player state is going to be removed from the party state. */
	FOnKronosPartyPlayerStateChanged& OnPlayerStateRemoved() { return OnPlayerStateRemovedEvent; }

	/** @return The delegate fired when a party chat message is received. */
	FOnKronosPartyChatMessageReceived& OnChatMessageReceived() { return OnChatMessageReceivedEvent; }

	/** @return The delegate fired when the party leader starts/stops matchmaking. */
	FOnKronosPartyLeaderMatchmaking& OnPartyLeaderMatchmaking() { return OnPartyLeaderMatchmakingEvent; }

	/** @return The delegate fired when we are following the party to a session. Only called for party clients. */
	FOnKronosFollowPartyStarted& OnFollowingPartyToSession() { return OnFollowingPartyToSessionEvent; }

	/** @return The delegate fired when there was a failure following the party to a session. */
	FOnKronosFollowPartyFailure& OnFollowPartyFailure() { return OnFollowPartyFailureEvent; }

	/** Dump current party state to the console. */
	virtual void DumpPartyState() const;

public:

	//~ Begin UObject interface
	virtual UWorld* GetWorld() const final; // Required by blueprints to have access to gameplay statics and other world context based nodes.
	//~ End UObject interface
};
