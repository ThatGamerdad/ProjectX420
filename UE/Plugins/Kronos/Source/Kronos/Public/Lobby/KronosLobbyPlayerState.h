// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "KronosLobbyPlayerState.generated.h"

/**
 * Delegate triggered when the lobby player's name changes.
 *
 * @param PlayerName The new player name.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosLobbyPlayerNameChanged, const FString&, PlayerName);

/**
 * Delegate triggered when the lobby player's ready state changes.
 *
 * @param bIsReady The new ready state.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosLobbyPlayerIsReadyChanged, bool, bIsReady);

/**
 * Delegate triggered when the lobby player's data changes.
 *
 * @param PlayerData The new player data.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosLobbyPlayerDataChanged, const TArray<int32>&, PlayerData);

/**
 * Delegate triggered when the player is leaving the lobby.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKronosLobbyPlayerDisconnecting);

/**
 * PlayerState class to be paired with other lobby classes.
 */
UCLASS()
class KRONOS_API AKronosLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	/** Event when the player's name changes. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnKronosLobbyPlayerNameChanged OnLobbyPlayerNameChanged;

	/** Event when the player's ready state changes. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnKronosLobbyPlayerIsReadyChanged OnLobbyPlayerIsReadyChanged;

	/** Event when the player's data changes. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnKronosLobbyPlayerDataChanged OnLobbyPlayerDataChanged;

	/** Event when the player is leaving the lobby. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnKronosLobbyPlayerDisconnecting OnLobbyPlayerDisconnecting;

protected:

	/** The replicated ready state of the player. */
	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bServerIsReady = false;

	/** The local ready state of the player. */
	bool bIsReady = false;

	/** The replicated player data. */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerData)
	TArray<int32> ServerPlayerData;

	/** The local player data. */
	TArray<int32> PlayerData;

public:

	/** Initialize the lobby player data. */
	virtual void InitPlayerData();

	/** Event when initializing the lobby player data. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "Init Player Data")
	void K2_InitPlayerData();

	/** Changes lobby player data. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void SetPlayerData(const TArray<int32>& NewPlayerData);

	/** Toggles player ready state. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void TogglePlayerIsReady();

	/** Changes player ready state. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void SetPlayerIsReady(bool bReady);

	/** Get the current lobby player data. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual TArray<int32> GetPlayerData() const { return PlayerData; }

	/** Get the ready state of the player. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual bool GetPlayerIsReady() const { return bIsReady; }

protected:

	/** Request a player data change on the server. */
	UFUNCTION(Server, Reliable)
	virtual void ServerSetPlayerData(const TArray<int32>& NewPlayerData);

	/** Request a ready state change on the server. */
	UFUNCTION(Server, Reliable)
	virtual void ServerSetPlayerIsReady(bool bReady);

	/** Called when the player data replicates from the server. */
	UFUNCTION()
	virtual void OnRep_PlayerData();

	/** Called when the player's ready state replicates from the server. */
	UFUNCTION()
	virtual void OnRep_IsReady();

public:

	//~ Begin APlayerState Interface
	virtual void PostInitializeComponents() override;
	virtual void ClientInitialize(class AController* C) override;
	virtual void OnRep_PlayerName() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End APlayerState Interface
};
