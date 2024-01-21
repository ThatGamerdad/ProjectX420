// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Lobby/KronosLobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

void AKronosLobbyPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Player data initialization for server (host) player.
	if (HasAuthority())
	{
		APlayerController* OwningController = Cast<APlayerController>(GetOwner());
		if (OwningController)
		{
			if (OwningController->IsLocalController())
			{
				InitPlayerData();
			}
		}
	}
}

void AKronosLobbyPlayerState::ClientInitialize(class AController* C)
{
	Super::ClientInitialize(C);

	// Player data initialization for client players. Only called on the local player.
	InitPlayerData();
}

void AKronosLobbyPlayerState::InitPlayerData()
{
	K2_InitPlayerData();
}

void AKronosLobbyPlayerState::SetPlayerData(const TArray<int32>& NewPlayerData)
{
	if (!HasAuthority())
	{
		PlayerData = NewPlayerData;
		OnLobbyPlayerDataChanged.Broadcast(PlayerData);

		ServerSetPlayerData(PlayerData);
		return;
	}

	ServerPlayerData = NewPlayerData;
	OnRep_PlayerData();
}

void AKronosLobbyPlayerState::TogglePlayerIsReady()
{
	bool bReady = GetPlayerIsReady();
	SetPlayerIsReady(!bReady);
}

void AKronosLobbyPlayerState::SetPlayerIsReady(bool bReady)
{
	if (!HasAuthority())
	{
		bIsReady = bReady;
		OnLobbyPlayerIsReadyChanged.Broadcast(bIsReady);

		ServerSetPlayerIsReady(bIsReady);
		return;
	}

	bServerIsReady = bReady;
	OnRep_IsReady();
}

void AKronosLobbyPlayerState::ServerSetPlayerData_Implementation(const TArray<int32>& NewPlayerData)
{
	SetPlayerData(NewPlayerData);
}

void AKronosLobbyPlayerState::ServerSetPlayerIsReady_Implementation(bool bReady)
{
	SetPlayerIsReady(bReady);
}

void AKronosLobbyPlayerState::OnRep_PlayerData()
{
	if (PlayerData != ServerPlayerData)
	{
		PlayerData = ServerPlayerData;
		OnLobbyPlayerDataChanged.Broadcast(PlayerData);
	}
}

void AKronosLobbyPlayerState::OnRep_IsReady()
{
	if (bIsReady != bServerIsReady)
	{
		bIsReady = bServerIsReady;
		OnLobbyPlayerIsReadyChanged.Broadcast(bIsReady);
	}
}

void AKronosLobbyPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	OnLobbyPlayerNameChanged.Broadcast(GetPlayerName());
}

void AKronosLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKronosLobbyPlayerState, bServerIsReady);
	DOREPLIFETIME(AKronosLobbyPlayerState, ServerPlayerData);
}
