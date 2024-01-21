// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Beacons/KronosPartyPlayerState.h"
#include "Beacons/KronosPartyClient.h"
#include "KronosPartyManager.h"
#include "Kronos.h"
#include "Net/UnrealNetwork.h"

void AKronosPartyPlayerState::ServerSetPlayerElo(int32 NewPlayerElo)
{
	PlayerElo = NewPlayerElo;
	OnRep_PlayerElo();
}

void AKronosPartyPlayerState::ServerSetPlayerData(const TArray<int32>& NewPlayerData)
{
	ServerPlayerData = NewPlayerData;
	OnRep_PlayerData();
}

void AKronosPartyPlayerState::ClientSetPlayerData(const TArray<int32>& NewPlayerData)
{
	PlayerData = NewPlayerData;

	K2_OnPlayerDataChanged(NewPlayerData);
	OnKronosPartyPlayerDataChanged().Broadcast(PlayerData);
}

void AKronosPartyPlayerState::SetPlayerActor(AKronosPartyPlayerActor* NewPlayerActor)
{
	PlayerActor = NewPlayerActor;
}

AKronosPartyClient* AKronosPartyPlayerState::GetOwningPartyClient() const
{
	return Cast<AKronosPartyClient>(ClientActor);
}

bool AKronosPartyPlayerState::IsLocalPlayer() const
{
	if (!UniqueId.IsValid())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosPartyPlayerState (%s): IsLocalPlayer() - UniqueId invalid!"), *DisplayName.ToString());
		return false;
	}

	const FUniqueNetIdRepl& PrimaryPlayerId = GetWorld()->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl();
	return UniqueId == PrimaryPlayerId;
}

bool AKronosPartyPlayerState::IsPartyLeader() const
{
	if (!UniqueId.IsValid())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosPartyPlayerState (%s): IsPartyLeader() - UniqueId invalid!"), *DisplayName.ToString());
		return false;
	}

	if (!PartyOwnerUniqueId.IsValid())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosPartyPlayerState (%s): IsPartyLeader() - PartyOwnerUniqueId invalid!"), *DisplayName.ToString());
		return false;
	}

	return UniqueId == PartyOwnerUniqueId;
}

void AKronosPartyPlayerState::SignalPartyOwnerChanged()
{
	OnPartyOwnerChanged().Broadcast(PartyOwnerUniqueId);
}

void AKronosPartyPlayerState::OnRep_PlayerElo()
{
	K2_OnPlayerEloChanged(PlayerElo);
	OnKronosPartyPlayerEloChanged().Broadcast(PlayerElo);
}

void AKronosPartyPlayerState::OnRep_PlayerData()
{
	if (PlayerData != ServerPlayerData)
	{
		PlayerData = ServerPlayerData;

		K2_OnPlayerDataChanged(PlayerData);
		OnKronosPartyPlayerDataChanged().Broadcast(PlayerData);
	}
}

void AKronosPartyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKronosPartyPlayerState, PlayerElo);
	DOREPLIFETIME(AKronosPartyPlayerState, ServerPlayerData);
}
