// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Lobby/KronosLobbyGameState.h"
#include "Lobby/KronosLobbyPlayerState.h"
#include "Kronos.h"
#include "GameFramework/PlayerStart.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UObjectIterator.h"

APlayerStart* AKronosLobbyGameState::FindPlayerStart(const APawn* PlayerPawn)
{
	// Lobby pawns will use this to find an unoccupied local or remote player start, where they can be moved to locally.

	// I'd like to improve this later, as this is bit of a "hacky" solution. Since our own pawn (the local) should always be in a fixed spot, we must relocate
	// players when they spawn in. We'll check each player start if it's occupied or not by checking if it has an owner. If not, we can spawn there. If it is occupied however, we will have to verify it first.
	// We have to do this because when a player disconnects, the ref to the pawn stored in that PlayerStart is still in memory (until garbage collection). So if a player joins before GC,
	// existing players will look at the PlayerStart and think that it is occupied (even though it's not), meaning they would fail to relocate the new player.

	if (PlayerPawn)
	{
		for (TObjectIterator<APlayerStart> It; It; ++It)
		{
			if (APlayerStart* PlayerStart = *It)
			{
				// Match player start and player pawn type (local or remote).
				if ((PlayerStart->PlayerStartTag == LocalPlayerStartTag) == PlayerPawn->IsLocallyControlled())
				{
					// Check if the player start is taken by another player.
					if (!PlayerStart->GetOwner())
					{
						// Player start is free, we can relocate the pawn without any issues.
						return PlayerStart;
					}

					// Safety measurement. Confirm that the player start is really taken.
					else
					{
						if (!GetWorld()->EncroachingBlockingGeometry(PlayerPawn, PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation()))
						{
							// Player start was in fact NOT taken. More info on why this happens above.
							return PlayerStart;
						}
					}
				}
			}
		}
	}

	UE_LOG(LogKronos, Warning, TEXT("No PlayerStart found for pawn %s"), PlayerPawn ? *PlayerPawn->GetName() : TEXT("null"));
	return nullptr;
}

int32 AKronosLobbyGameState::GetNumReadyPlayers() const
{
	int32 NumReadyPlayers = 0;
	for (APlayerState* PlayerState : PlayerArray)
	{
		AKronosLobbyPlayerState* LobbyPlayerState = Cast<AKronosLobbyPlayerState>(PlayerState);
		if (LobbyPlayerState)
		{
			if (LobbyPlayerState->GetPlayerIsReady())
			{
				NumReadyPlayers++;
			}
		}
	}

	return NumReadyPlayers;
}

void AKronosLobbyGameState::SetLobbyState(const EKronosLobbyState InLobbyState)
{
	if (HasAuthority() && InLobbyState != LobbyState)
	{
		LobbyState = InLobbyState;
		OnRep_LobbyState();
	}
}

void AKronosLobbyGameState::SetLobbyCountdownTime(const int32 InLobbyCountdownTime)
{
	if (HasAuthority() && InLobbyCountdownTime != LobbyTimer)
	{
		LobbyTimer = InLobbyCountdownTime;
		OnRep_LobbyCountdownTime();
	}
}

void AKronosLobbyGameState::OnRep_LobbyState()
{
	OnLobbyStateChanged.Broadcast(LobbyState);
	OnLobbyUpdated.Broadcast(LobbyState, LobbyTimer);
}

void AKronosLobbyGameState::OnRep_LobbyCountdownTime()
{
	OnLobbyUpdated.Broadcast(LobbyState, LobbyTimer);
}

void AKronosLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	
	AKronosLobbyPlayerState* LobbyPlayerState = Cast<AKronosLobbyPlayerState>(PlayerState);
	if (LobbyPlayerState)
	{
		OnPlayerConnectedToLobby.Broadcast(LobbyPlayerState);
	}
}

void AKronosLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	AKronosLobbyPlayerState* LobbyPlayerState = Cast<AKronosLobbyPlayerState>(PlayerState);
	if (LobbyPlayerState)
	{
		LobbyPlayerState->OnLobbyPlayerDisconnecting.Broadcast();
		OnPlayerDisconnectedFromLobby.Broadcast(LobbyPlayerState);
	}

	Super::RemovePlayerState(PlayerState);
}

void AKronosLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKronosLobbyGameState, LobbyState);
	DOREPLIFETIME(AKronosLobbyGameState, LobbyTimer);
}
