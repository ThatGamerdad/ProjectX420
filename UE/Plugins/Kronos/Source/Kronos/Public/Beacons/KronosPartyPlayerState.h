// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LobbyBeaconPlayerState.h"
#include "KronosPartyPlayerState.generated.h"

class AKronosPartyPlayerActor;

/**
 * Delegate triggered when the party player's elo score changes.
 *
 * @param PlayerElo The new player elo score.
 */
DECLARE_EVENT_OneParam(AKronosPartyPlayerState, FOnKronosPartyPlayerEloChanged, const int32& /** PlayerElo */);

/**
 * Delegate triggered when the party player's data changes.
 *
 * @param PlayerData The new player data.
 */
DECLARE_EVENT_OneParam(AKronosPartyPlayerState, FOnKronosPartyPlayerDataChanged, const TArray<int32>& /** PlayerData */);

/**
 * Lightweight representation of a player while connected to a party.
 * There will be one of these for each party member while connected to a party.
 */
UCLASS(Blueprintable, BlueprintType)
class KRONOS_API AKronosPartyPlayerState : public ALobbyBeaconPlayerState
{
	GENERATED_BODY()

public:

	/** Elo score representing the player's skill level. */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerElo)
	int32 PlayerElo;

	/** The replicated player data. */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerData)
	TArray<int32> ServerPlayerData;

	/**
	 * An actor representing this player in the world.
	 * Used to make the player physically appear in the world, similar to how lobbies look like.
	 * PartyPlayerActors are not replicated since their only purpose is to visualize the data of this player.
	 */
	UPROPERTY(Transient)
	AKronosPartyPlayerActor* PlayerActor;

	/** The local player data. */
	TArray<int32> PlayerData;

private:

	/** Delegate triggered when the elo score changes for this player. */
	FOnKronosPartyPlayerEloChanged OnPartyPlayerEloChanged;

	/** Delegate triggered when the player data changes for this player. */
	FOnKronosPartyPlayerDataChanged OnPartyPlayerDataChanged;

public:

	/**
	 * Called when the player elo is being set by the server.
	 * 
	 * Do not call this directly. Use KronosPartyClient::SetPlayerElo() instead.
	 *
	 * NOTE: This is not an RPC! BeaconPlayerStates don't have RPC capabilities. The transition from client to server is done in the owning KronosPartyClient.
	 */
	virtual void ServerSetPlayerElo(int32 NewPlayerElo);

	/**
	 * Called when the player data is being set by the server.
	 * At this point ClientSetPlayerData() was already called client side before requesting the change with the server.
	 * The actual player data is set here and the value will replicate down to the client, correcting any differences.
	 *
	 * Do not call this directly. Use KronosPartyClient::SetPlayerData() instead.
	 * 
	 * NOTE: This is not an RPC! BeaconPlayerStates don't have RPC capabilities. The transition from client to server is done in the owning KronosPartyClient.
	 */
	virtual void ServerSetPlayerData(const TArray<int32>& NewPlayerData);

	/**
	 * Called by the owning KronosPartyClient before sending the player data to the server.
	 * We change the player data locally on the client side so that we can see the changes immediately.
	 * The server is still in authority! The actual player data will replicate down to the client, correcting any differences.
	 *
	 * Do not call this directly. Use KronosPartyClient::SetPlayerData() instead.
	 * 
	 * NOTE: This is not an RPC! BeaconPlayerStates don't have RPC capabilities. The transition from client to server is done in the owning KronosPartyClient.
	 */
	virtual void ClientSetPlayerData(const TArray<int32>& NewPlayerData);

	/**
	 * Called by the KronosPartyState when a player actor was created or being destroyed for this player.
	 * The actor is already owned by the player state when this is called.
	 */
	virtual void SetPlayerActor(AKronosPartyPlayerActor* NewPlayerActor);

	/** Get the player's client actor. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual AKronosPartyClient* GetOwningPartyClient() const;

	/** Get the player's display name. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual FText GetPlayerName() const { return DisplayName; }

	/** Get the player's unique id. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual FUniqueNetIdRepl GetPlayerId() const { return UniqueId; }

	/** Get the party leader's unique id. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual FUniqueNetIdRepl GetPartyLeaderId() const { return PartyOwnerUniqueId; }

	/** Get the current player elo score. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetPlayerElo() const { return PlayerElo; }

	/** Get the player's actor. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual AKronosPartyPlayerActor* GetPlayerActor() const { return PlayerActor; }

	/** Get the current player data. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual TArray<int32> GetPlayerData() const { return PlayerData; }

	/** Check whether this player is the local player or not. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual bool IsLocalPlayer() const;

	/** Check whether this player is the party leader or not. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual bool IsPartyLeader() const;

	/** @return The delegate fired when the elo score changes for this player. */
	FOnKronosPartyPlayerEloChanged& OnKronosPartyPlayerEloChanged() { return OnPartyPlayerEloChanged; }

	/** @return The delegate fired when the player data changes for this player. */
	FOnKronosPartyPlayerDataChanged& OnKronosPartyPlayerDataChanged() { return OnPartyPlayerDataChanged; }

protected:

	/** Event when the elo score changes for this player. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Elo Changed")
	void K2_OnPlayerEloChanged(const int32 NewPlayerElo);

	/** Event when the player data changes for this player. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Data Changed")
	void K2_OnPlayerDataChanged(const TArray<int32>& NewPlayerData);

protected:

	/** Signals party owner changed delegate. */
	virtual void SignalPartyOwnerChanged();

	friend class AKronosPartyHost;

protected:

	/** Called when the elo score replicates from the server. */
	UFUNCTION()
	virtual void OnRep_PlayerElo();

	/** Called when the player data replicates from the server. */
	UFUNCTION()
	virtual void OnRep_PlayerData();

public:

	//~ Begin ALobbyBeaconPlayerState Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End ALobbyBeaconPlayerState Interface
};
