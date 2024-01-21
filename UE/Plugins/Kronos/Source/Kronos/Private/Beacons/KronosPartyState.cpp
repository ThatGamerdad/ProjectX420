// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Beacons/KronosPartyState.h"
#include "Beacons/KronosPartyClient.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "KronosPartyPlayerActor.h"
#include "KronosPartyManager.h"
#include "KronosConfig.h"
#include "Kronos.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AKronosPartyState::AKronosPartyState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	LobbyBeaconPlayerStateClass = GetDefault<UKronosConfig>()->PartyPlayerStateClass;

	OnPlayerLobbyStateAdded().AddUObject(this, &ThisClass::OnPlayerStateAdded);
	OnPlayerLobbyStateRemoved().AddUObject(this, &ThisClass::OnPlayerStateRemoved);
}

void AKronosPartyState::ServerSetPartyLeaderMatchmaking(bool bMatchmaking)
{
	if (bPartyLeaderMatchmaking != bMatchmaking)
	{
		bPartyLeaderMatchmaking = bMatchmaking;
		OnRep_PartyLeaderMatchmaking();
	}
}

AKronosPartyClient* AKronosPartyState::GetPartyClient(const FUniqueNetIdRepl& PlayerId)
{
	ALobbyBeaconPlayerState* PlayerState = GetPlayer(PlayerId);
	if (PlayerState)
	{
		return CastChecked<AKronosPartyClient>(PlayerState->ClientActor);
	}

	return nullptr;
}

AKronosPartyPlayerState* AKronosPartyState::GetPartyPlayerState(const FUniqueNetIdRepl& PlayerId)
{
	ALobbyBeaconPlayerState* PlayerState = GetPlayer(PlayerId);
	if (PlayerState)
	{
		return Cast<AKronosPartyPlayerState>(PlayerState);
	}

	return nullptr;
}

TArray<AKronosPartyClient*> AKronosPartyState::GetPartyClients()
{
	TArray<AKronosPartyClient*> OutPartyPlayers;
	for (FLobbyPlayerStateActorInfo& LobbyPlayerInfo : Players.GetAllPlayers())
	{
		AKronosPartyClient* PartyPlayer = Cast<AKronosPartyClient>(LobbyPlayerInfo.LobbyPlayerState->ClientActor);
		if (PartyPlayer)
		{
			OutPartyPlayers.Add(PartyPlayer);
		}
	}

	return OutPartyPlayers;
}

TArray<AKronosPartyPlayerState*> AKronosPartyState::GetPartyPlayerStates()
{
	TArray<AKronosPartyPlayerState*> OutPlayerStates;
	for (FLobbyPlayerStateActorInfo& LobbyPlayerInfo : Players.GetAllPlayers())
	{
		AKronosPartyPlayerState* PartyPlayerState = Cast<AKronosPartyPlayerState>(LobbyPlayerInfo.LobbyPlayerState);
		if (PartyPlayerState)
		{
			OutPlayerStates.Add(PartyPlayerState);
		}
	}

	return OutPlayerStates;
}

TArray<FUniqueNetIdRepl> AKronosPartyState::GetPartyPlayerUniqueIds()
{
	TArray<FUniqueNetIdRepl> OutUniqueIds;
	for (FLobbyPlayerStateActorInfo& LobbyPlayerInfo : Players.GetAllPlayers())
	{
		ALobbyBeaconPlayerState* PlayerState = LobbyPlayerInfo.LobbyPlayerState;
		if (PlayerState)
		{
			if (PlayerState->UniqueId.IsValid())
			{
				OutUniqueIds.Add(PlayerState->UniqueId);
			}
		}
	}

	return OutUniqueIds;
}

int32 AKronosPartyState::GetPartyEloAverage()
{
	TArray<FLobbyPlayerStateActorInfo> PartyMembers = Players.GetAllPlayers();
	int32 EloSum = 0;

	for (FLobbyPlayerStateActorInfo& PlayerInfo : PartyMembers)
	{
		AKronosPartyPlayerState* PartyPlayerState = Cast<AKronosPartyPlayerState>(PlayerInfo.LobbyPlayerState);
		if (PartyPlayerState)
		{
			EloSum += PartyPlayerState->GetPlayerElo();
		}
	}

	return EloSum / PartyMembers.Num();
}

void AKronosPartyState::OnPlayerStateAdded(ALobbyBeaconPlayerState* PlayerState)
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyState: PlayerState added."));

	DumpState();

	AKronosPartyPlayerState* PartyPlayerState = Cast<AKronosPartyPlayerState>(PlayerState);
	if (PartyPlayerState)
	{
		// Create a player actor if needed.
		if (PartyPlayerActorClass.Get())
		{
			CreatePartyPlayerActor(PartyPlayerState);
		}

		// Give blueprints a chance to implement custom logic.
		K2_OnPlayerStateAdded(PartyPlayerState);

		UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
		PartyManager->OnPlayerStateAdded().Broadcast(PartyPlayerState);
	}
}

void AKronosPartyState::OnPlayerStateRemoved(ALobbyBeaconPlayerState* PlayerState)
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyState: PlayerState removed."));

	AKronosPartyPlayerState* PartyPlayerState = Cast<AKronosPartyPlayerState>(PlayerState);
	if (PartyPlayerState)
	{
		// Destroy the player actor if there's one.
		AKronosPartyPlayerActor* PlayerActor = PartyPlayerState->GetPlayerActor();
		if (PlayerActor)
		{
			PartyPlayerState->SetPlayerActor(nullptr);
			PlayerActor->Destroy();
		}

		// Give blueprints a chance to implement custom logic.
		K2_OnPlayerStateRemoved(PartyPlayerState);

		UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
		PartyManager->OnPlayerStateRemoved().Broadcast(PartyPlayerState);
	}
}

void AKronosPartyState::CreatePartyPlayerActor(AKronosPartyPlayerState* OwningPlayerState)
{
	if (!OwningPlayerState)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyState: Attempted to create player actor for nullptr player state."));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwningPlayerState;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Create and assign a new player actor to the given player.
	// These actors are not replicated so we don't have to worry about only spawning on server.
	AKronosPartyPlayerActor* NewPlayerActor = GetWorld()->SpawnActor<AKronosPartyPlayerActor>(PartyPlayerActorClass, FTransform::Identity, SpawnParams);
	OwningPlayerState->SetPlayerActor(NewPlayerActor);
}

void AKronosPartyState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Cleanup party state before destroying it.
	CleanupPartyState();

	Super::EndPlay(EndPlayReason);
}

void AKronosPartyState::CleanupPartyState()
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyState: CleanupPartyState"));

	// Destroy all party actors since we are about to leave the party.
	// Have to do this manually because player state removed events are not called when leaving the party. 

	for (AKronosPartyPlayerState* PlayerState : GetPartyPlayerStates())
	{
		AKronosPartyPlayerActor* PlayerActor = PlayerState->GetPlayerActor();
		if (IsValid(PlayerActor))
		{
			PlayerActor->Destroy();
		}
	}
}

void AKronosPartyState::OnRep_PartyLeaderMatchmaking()
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	PartyManager->OnPartyLeaderMatchmaking().Broadcast(bPartyLeaderMatchmaking);
}

void AKronosPartyState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKronosPartyState, bPartyLeaderMatchmaking);
}
