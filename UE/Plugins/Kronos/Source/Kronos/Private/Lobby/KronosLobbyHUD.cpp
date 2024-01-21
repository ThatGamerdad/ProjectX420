// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Lobby/KronosLobbyHUD.h"
#include "Lobby/KronosLobbyGameState.h"
#include "Lobby/KronosLobbyPlayerState.h"

void AKronosLobbyHUD::BeginPlay()
{
	Super::BeginPlay();

	// Begin waiting initial replication.
	WaitInitialReplication();
}

void AKronosLobbyHUD::WaitInitialReplication()
{
	// Check if initial object replication finished or not.
	if (HasInitialReplicationFinished())
	{
		OnInitialReplicationFinished();
		return;
	}

	// We are still waiting for initial object replication. Checking again in the next frame...
	else
	{
		if (IsValid(this))
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::WaitInitialReplication);
			return;
		}
	}
}

bool AKronosLobbyHUD::HasInitialReplicationFinished_Implementation() const
{
	AGameStateBase* GameState = GetWorld()->GetGameState();
	return GameState != nullptr;
}

void AKronosLobbyHUD::OnInitialReplicationFinished()
{
	AKronosLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AKronosLobbyGameState>();
	if (LobbyGameState)
	{
		for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
		{
			AKronosLobbyPlayerState* LobbyPlayerState = Cast<AKronosLobbyPlayerState>(PlayerState);
			if (LobbyPlayerState)
			{
				OnPlayerJoinedLobby(LobbyPlayerState);
			}
		}

		LobbyGameState->OnPlayerConnectedToLobby.AddDynamic(this, &ThisClass::OnPlayerJoinedLobby);
		LobbyGameState->OnPlayerDisconnectedFromLobby.AddDynamic(this, &ThisClass::OnPlayerLeftLobby);
	}

	K2_OnInitialReplicationFinished();
}

void AKronosLobbyHUD::OnPlayerJoinedLobby(AKronosLobbyPlayerState* PlayerState)
{
	K2_OnPlayerJoinedLobby(PlayerState);
}

void AKronosLobbyHUD::OnPlayerLeftLobby(AKronosLobbyPlayerState* PlayerState)
{
	K2_OnPlayerLeftLobby(PlayerState);
}
