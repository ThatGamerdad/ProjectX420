// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "KronosLobbyGameState.generated.h"

class AKronosLobbyPlayerState;
class APlayerStart;
enum class EKronosLobbyState : uint8;

/**
 * Delegate triggered when a new player joins the lobby.
 *
 * @param PlayerState Player state of the new player.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerConnectedToKronosLobby, AKronosLobbyPlayerState*, PlayerState);

/**
 * Delegate triggered when an existing player leaves the lobby.
 *
 * @param PlayerState Player state of the player leaving the lobby.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDisconnectedFromKronosLobby, AKronosLobbyPlayerState*, PlayerState);

/**
 * Delegate triggered when the lobby state changes.
 *
 * @param LobbyState The new lobby state.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosLobbyStateChanged, const EKronosLobbyState, LobbyState);

/**
 * Delegate triggered when either the lobby state or time changes.
 *
 * @param LobbyState The new lobby state.
 * @param LobbyCountdownTime The new lobby countdown time.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKronosLobbyUpdated, const EKronosLobbyState, LobbyState, const int32, LobbyCountdownTime);

/**
 * Game state to be paired with the LobbyGameMode class.
 */
UCLASS()
class KRONOS_API AKronosLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	/** Whether to move lobby players around locally so that the local player can be in a fix spot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby")
	bool bRelocatePlayers = true;

	/** Tag of the player start where the local player should be moved to. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby", meta = (EditCondition = "bRelocatePlayers"))
	FName LocalPlayerStartTag = FName(TEXT("Local"));

public:

	/** Event when a new player joins the lobby. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerConnectedToKronosLobby OnPlayerConnectedToLobby;

	/** Event when an existing player leaves the lobby. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerDisconnectedFromKronosLobby OnPlayerDisconnectedFromLobby;

	/** Event when the lobby state changes. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnKronosLobbyStateChanged OnLobbyStateChanged;

	/** Event when either the lobby state or time changes. Helper delegate for UI widgets. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnKronosLobbyUpdated OnLobbyUpdated;

protected:

	/** The current lobby state. */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState)
	EKronosLobbyState LobbyState;

	/** The current lobby countdown time. */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyCountdownTime)
	int32 LobbyTimer;

public:

	/** Find a new player start for the given pawn. This is used when we want to have the local player in a fix spot. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual APlayerStart* FindPlayerStart(const APawn* PlayerPawn);

	/** Get the current lobby state. */
	UFUNCTION(BlueprintPure, Category = "Default")
	EKronosLobbyState GetLobbyState() const { return LobbyState; }

	/** Get the current lobby countdown time. */
	UFUNCTION(BlueprintPure, Category = "Default")
	int32 GetLobbyCountdownTime() const { return LobbyTimer; }

	/** Get the number of players in the lobby. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetNumPlayers() const { return PlayerArray.Num(); }

	/** Get the number of players who are ready. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetNumReadyPlayers() const;

	/** Check whether all players are ready or not. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual bool IsEveryPlayerReady() const { return GetNumPlayers() == GetNumReadyPlayers(); }

protected:

	/** Changes lobby state. Called by the game mode. */
	virtual void SetLobbyState(const EKronosLobbyState InLobbyState);

	/** Changes lobby countdown time. Called by the game mode. */
	virtual void SetLobbyCountdownTime(const int32 InLobbyCountdownTime);

	friend class AKronosLobbyGameMode;

protected:

	/** Called when the lobby state replicates. */
	UFUNCTION()
	virtual void OnRep_LobbyState();

	/** Called when the lobby countdown time replicates. */
	UFUNCTION()
	virtual void OnRep_LobbyCountdownTime();

public:

	//~ Begin AGameStateBase Interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AGameStateBase Interface
};
