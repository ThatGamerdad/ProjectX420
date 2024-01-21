// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LobbyBeaconState.h"
#include "GameFramework/OnlineReplStructs.h"
#include "KronosPartyState.generated.h"

class AKronosPartyClient;
class AKronosPartyPlayerState;
class AKronosPartyPlayerActor;

/**
 * Shared state of the party. Exists on both the server and the client.
 * Similar to the game state in the Unreal networking architecture.
 */
UCLASS(Blueprintable, BlueprintType)
class KRONOS_API AKronosPartyState : public ALobbyBeaconState
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	AKronosPartyState(const FObjectInitializer& ObjectInitializer);

public:

	/**
	 * Optional actor to spawn for each party member.
	 * Use KronosPartyPlayerStart actors to mark spawn locations for these actors.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Party")
	TSubclassOf<AKronosPartyPlayerActor> PartyPlayerActorClass;

protected:

	/** Whether the party leader is matchmaking or not. */
	UPROPERTY(ReplicatedUsing = OnRep_PartyLeaderMatchmaking)
	bool bPartyLeaderMatchmaking;

public:

	/**
	 * Set whether the party leader is matchmaking or not. Should only be called from the server.
	 * NOTE: This is not an RPC! LobbyBeaconStates don't have RPC capabilities. If client->server transition is needed, the party client actor should be used.
	 */
	virtual void ServerSetPartyLeaderMatchmaking(bool bMatchmaking);

	/**
	 * Clean up the party state.
	 * Called before the actor is destroyed.
	 */
	virtual void CleanupPartyState();

	/** Check whether the party leader is matchmaking or not. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual bool IsPartyLeaderMatchmaking() { return bPartyLeaderMatchmaking; }

	/** Get an existing player in the party. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual AKronosPartyClient* GetPartyClient(const FUniqueNetIdRepl& PlayerId);

	/** Get an existing player's state in the party. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual AKronosPartyPlayerState* GetPartyPlayerState(const FUniqueNetIdRepl& PlayerId);

	/** Get all existing players in the party. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual TArray<AKronosPartyClient*> GetPartyClients();

	/** Get all party player states. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual TArray<AKronosPartyPlayerState*> GetPartyPlayerStates();

	/** Get all party players unique id. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual TArray<FUniqueNetIdRepl> GetPartyPlayerUniqueIds();
	
	/** Get the average elo score of the party. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetPartyEloAverage();

protected:

	/** Called after a player state has been added to the party. */
	virtual void OnPlayerStateAdded(ALobbyBeaconPlayerState* PlayerState);

	/** Called before a player state is removed from the party. */
	virtual void OnPlayerStateRemoved(ALobbyBeaconPlayerState* PlayerState);

	/**
	 * Create a party player actor for the given player.
	 * Called during OnPlayerStateAdded() if the PartyPlayerActorClass is not null.
	 */
	virtual void CreatePartyPlayerActor(AKronosPartyPlayerState* OwningPlayerState);

	/** Called when the party leader matchmaking state replicates from the server. */
	UFUNCTION()
	virtual void OnRep_PartyLeaderMatchmaking();

protected:

	/** Event after a player state has been added to the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player State Added")
	void K2_OnPlayerStateAdded(AKronosPartyPlayerState* PlayerState);

	/** Event before a player state is removed from the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player State Removed")
	void K2_OnPlayerStateRemoved(AKronosPartyPlayerState* PlayerState);

public:

	//~ Begin ALobbyBeaconState interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End ALobbyBeaconState interface
};
