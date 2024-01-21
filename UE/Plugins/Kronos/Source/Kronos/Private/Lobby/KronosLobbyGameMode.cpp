// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Lobby/KronosLobbyGameMode.h"
#include "Lobby/KronosLobbyGameState.h"
#include "Lobby/KronosLobbyPlayerController.h"
#include "Lobby/KronosLobbyPlayerState.h"
#include "Lobby/KronosLobbyHUD.h"
#include "Lobby/KronosLobbyPawn.h"
#include "KronosReservationManager.h"
#include "KronosOnlineSession.h"
#include "Kronos.h"
#include "GameFramework/GameSession.h"
#include "TimerManager.h"

AKronosLobbyGameMode::AKronosLobbyGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	GameStateClass = AKronosLobbyGameState::StaticClass();
	PlayerControllerClass = AKronosLobbyPlayerController::StaticClass();
	PlayerStateClass = AKronosLobbyPlayerState::StaticClass();
	HUDClass = AKronosLobbyHUD::StaticClass();
	DefaultPawnClass = AKronosLobbyPawn::StaticClass();
	bUseSeamlessTravel = true;
}

void AKronosLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	InitializeLobby();
}

void AKronosLobbyGameMode::InitializeLobby()
{
	AKronosLobbyGameState* KronosLobbyGameState = GetGameState<AKronosLobbyGameState>();
	if (KronosLobbyGameState)
	{
		LobbyGameState = KronosLobbyGameState;

		// Give blueprints a chance to implement custom logic.
		K2_InitializeLobby();

		// Make sure that NumPlayersRequired can be reached.
		if (NumPlayersRequired > GameSession->MaxPlayers)
		{
			UE_LOG(LogKronos, Warning, TEXT("NumPlayersRequired in lobby is more than the max player count. Reducing NumPlayersRequired to max player count."));
			NumPlayersRequired = GameSession->MaxPlayers;
		}

		SetLobbyState(EKronosLobbyState::WaitingForPlayers);

		GetWorldTimerManager().SetTimer(TimerHandle_TickLobby, this, &ThisClass::TickLobby, 1.0, true);
		return;
	}

	UE_LOG(LogKronos, Error, TEXT("Failed to initialize lobby. No KronosLobbyGameState was found. Make sure that you have it set as your GameState class!"));
}

void AKronosLobbyGameMode::TickLobby()
{
	switch (LobbyState)
	{
	case EKronosLobbyState::WaitingForPlayers:
		HandleWaitingForPlayers();
		break;
	case EKronosLobbyState::WaitingToStart:
		HandleWaitingToStart();
		break;
	case EKronosLobbyState::StartingMatch:
		HandleStartingMatch();
		break;
	}

	K2_TickLobby();
}

void AKronosLobbyGameMode::HandleWaitingForPlayers()
{
	// Enough players joined the lobby, start match countdown.
	if (LobbyGameState->GetNumPlayers() >= GetNumPlayersRequired())
	{
		SetLobbyState(EKronosLobbyState::WaitingToStart, LobbyCountdownTime);
		return;
	}
}

void AKronosLobbyGameMode::HandleWaitingToStart()
{
	// Not enough players are present, return to waiting for players.
	if (LobbyGameState->GetNumPlayers() < GetNumPlayersRequired())
	{
		SetLobbyState(EKronosLobbyState::WaitingForPlayers);
		return;
	}

	if (!bCountdownOnlyIfEveryoneReady)
	{
		// Update lobby countdown time.
		SetLobbyTimer(LobbyTimer - 1);

		// Lobby timer hasn't reached critical point yet. Waiting for players to ready.
		if (LobbyTimer > LobbyFinalCountdownTime)
		{
			// All players are ready, start final countdown.
			if (LobbyGameState->IsEveryPlayerReady())
			{
				SetLobbyState(EKronosLobbyState::StartingMatch, LobbyFinalCountdownTime);
				return;
			}
		}

		// Lobby timer reached critical point! Force all players ready and start final countdown.
		if (LobbyTimer <= LobbyFinalCountdownTime)
		{
			SetAllPlayersReady();

			SetLobbyState(EKronosLobbyState::StartingMatch, LobbyFinalCountdownTime);
			return;
		}
	}
	
	else
	{
		// All players are ready, start final countdown.
		if (LobbyGameState->IsEveryPlayerReady())
		{
			SetLobbyState(EKronosLobbyState::StartingMatch, LobbyFinalCountdownTime);
			return;
		}
	}
}

void AKronosLobbyGameMode::HandleStartingMatch()
{
	// Not enough players are present, return to waiting for players.
	if (LobbyGameState->GetNumPlayers() < GetNumPlayersRequired())
	{
		SetLobbyState(EKronosLobbyState::WaitingForPlayers);
		return;
	}

	// Update lobby countdown time.
	SetLobbyTimer(LobbyTimer - 1);

	// Lobby timer reached zero, start the match.
	if (LobbyTimer == 0)
	{
		StartMatch();
		return;
	}
}

void AKronosLobbyGameMode::StartMatch()
{
	// Give the game session a chance to abort starting the match.
	if (GameSession->HandleStartMatchRequest())
	{
		UE_LOG(LogKronos, Warning, TEXT("GameSession handled StartMatch request. Start match call aborted."));
		return;
	}

	GameSession->HandleMatchHasStarted();

	SetLobbyTimer(0);
	SetLobbyState(EKronosLobbyState::MatchStarted);

	OnMatchStarted();
}

void AKronosLobbyGameMode::OnMatchStarted()
{
	K2_OnMatchStarted();
}

void AKronosLobbyGameMode::SetAllPlayersReady()
{
	// Loop through the players and set their ready states to true.
	for (APlayerState* Player : LobbyGameState->PlayerArray)
	{
		AKronosLobbyPlayerState* LobbyPlayer = Cast<AKronosLobbyPlayerState>(Player);
		if (LobbyPlayer)
		{
			if (!LobbyPlayer->GetPlayerIsReady())
			{
				LobbyPlayer->SetPlayerIsReady(true);
			}
		}
	}
}

void AKronosLobbyGameMode::SetLobbyState(const EKronosLobbyState InLobbyState, const int32 InCountdownTime)
{
	LobbyState = InLobbyState;
	LobbyGameState->SetLobbyState(LobbyState);

	if (InCountdownTime >= 0)
	{
		SetLobbyTimer(InCountdownTime);
	}
}

void AKronosLobbyGameMode::SetLobbyTimer(const int32 InCountdownTime)
{
	LobbyTimer = FMath::Max(InCountdownTime, 0);
	LobbyGameState->SetLobbyCountdownTime(LobbyTimer);
}

int32 AKronosLobbyGameMode::GetNumPlayersRequired() const
{
#if WITH_EDITOR
	return NumPlayersRequiredInEditor;
#else
	return NumPlayersRequired;
#endif
}
