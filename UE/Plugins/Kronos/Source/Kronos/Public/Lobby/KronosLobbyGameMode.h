// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KronosLobbyGameMode.generated.h"

/**
 * Possible lobby states.
 */
UENUM(BlueprintType)
enum class EKronosLobbyState : uint8
{
	/** Lobby is initializing. */
	Initializing,

	/** Lobby is waiting for more players. */
	WaitingForPlayers,

	/** Lobby is waiting for all players to ready. Countdown has started. */
	WaitingToStart,

	/** Lobby is starting. Final countdown started. */
	StartingMatch,

	/** Lobby started. Countdown reached zero, and we are ready to travel to the game map. */
	MatchStarted,
};

/**
 * Game mode that is set up to behave like a game lobby.
 */
UCLASS()
class KRONOS_API AKronosLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	/** Default constructor. */
	AKronosLobbyGameMode(const FObjectInitializer& ObjectInitializer);

public:

	/** Number of players required to start. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby", meta = (ClampMin = "0"))
	int32 NumPlayersRequired = 2;

	/** Number of players required to start while testing in the editor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby", meta = (ClampMin = "0"))
	int32 NumPlayersRequiredInEditor = 2;

	/** Amount of time to wait for all players to ready before starting anyways. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby", meta = (ClampMin = "0"))
	int32 LobbyCountdownTime = 60;

	/**
	 * Amount of time to wait before actually starting.
	 * Should be less than the LobbyCountdownTime.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby", meta = (ClampMin = "0"))
	int32 LobbyFinalCountdownTime = 5;

	/** Whether to start lobby countdown time only if all players are ready. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lobby")
	bool bCountdownOnlyIfEveryoneReady = false;

protected:

	/**	Cached lobby state for ease of access. */
	UPROPERTY(Transient)
	class AKronosLobbyGameState* LobbyGameState;

	/** The current state of the lobby. */
	EKronosLobbyState LobbyState;

	/** The current countdown time of the lobby. */
	int32 LobbyTimer;

	/** Handle used to tick the lobby. Runs on a one sec timer. */
	FTimerHandle TimerHandle_TickLobby;

public:

	/** Starts the lobby. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void StartMatch();

	/** Forces all players to be ready. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetAllPlayersReady();

	/** Changes lobby state. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetLobbyState(const EKronosLobbyState InLobbyState, const int32 InCountdownTime = -1);

	/** Changes lobby countdown time. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetLobbyTimer(const int32 InCountdownTime);

	/** Get the current lobby state. */
	UFUNCTION(BlueprintPure, Category = "Default")
	EKronosLobbyState GetLobbyState() const { return LobbyState; }

	/** Get the current lobby countdown time. */
	UFUNCTION(BlueprintPure, Category = "Default")
	int32 GetLobbyTimer() const { return LobbyTimer; }

	/**
	 * Get the number of players required to start.
	 * This function is context aware. It will return the NumPlayersRequiredInEditor while testing in the editor.
	 */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetNumPlayersRequired() const;

protected:

	/** Initializes the lobby. */
	virtual void InitializeLobby();

	/** Called when the lobby ticks (once every sec). */
	virtual void TickLobby();

	/** Called when the lobby ticks while waiting for players. */
	virtual void HandleWaitingForPlayers();

	/** Called when the lobby ticks while waiting to start. */
	virtual void HandleWaitingToStart();

	/** Called when the lobby ticks while the lobby is starting- */
	virtual void HandleStartingMatch();

	/** Called when the lobby started. */
	virtual void OnMatchStarted();

protected:

	/** Event when the lobby is initializing. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "Initialize Lobby")
	void K2_InitializeLobby();

	/** Event when the lobby ticks (once every sec). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "Tick Lobby")
	void K2_TickLobby();

	/** Event when the lobby is started. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Match Started")
	void K2_OnMatchStarted();

public:

	//~ Begin AGameModeBase Interface
	virtual void BeginPlay() override;
	//~ End AGameModeBase Interface
};
