// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Beacons/KronosPartyClient.h"
#include "Beacons/KronosPartyHost.h"
#include "Beacons/KronosPartyState.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "KronosMatchmakingManager.h"
#include "KronosPartyManager.h"
#include "KronosOnlineSession.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AKronosPartyClient::AKronosPartyClient(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnLoginComplete().BindUObject(this, &ThisClass::OnPartyLoginComplete);
		OnPlayerJoined().BindUObject(this, &ThisClass::OnPartyPlayerJoined);
		OnPlayerLeft().BindUObject(this, &ThisClass::OnPartyPlayerLeft);
		OnJoiningGame().BindUObject(this, &ThisClass::HandleJoiningGame);
		OnJoiningGameAck().BindUObject(this, &ThisClass::HandleJoiningGameAck);
	}
}

void AKronosPartyClient::ServerInitPlayer()
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyClient: ServerInitPlayer"));
	K2_ServerInitPlayer();
}

void AKronosPartyClient::ClientInitPlayer()
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyClient: ClientInitPlayer"));
	K2_ClientInitPlayer();
}

void AKronosPartyClient::SetPlayerElo(int32 NewPlayerElo)
{
	AKronosPartyHost* PartyHost = Cast<AKronosPartyHost>(GetBeaconOwner());
	if (PartyHost)
	{
		PartyHost->ProcessPlayerEloChange(this, NewPlayerElo);
		return;
	}

	ServerSetPlayerElo(NewPlayerElo);
}

void AKronosPartyClient::ServerSetPlayerElo_Implementation(const int32 NewPlayerElo)
{
	SetPlayerElo(NewPlayerElo);
}

bool AKronosPartyClient::ServerSetPlayerElo_Validate(const int32 NewPlayerElo)
{
	return true;
}

void AKronosPartyClient::SetPlayerData(const TArray<int32>& NewPlayerData)
{
	AKronosPartyHost* PartyHost = Cast<AKronosPartyHost>(GetBeaconOwner());
	if (!PartyHost)
	{
		AKronosPartyPlayerState* PartyPlayerState = CastChecked<AKronosPartyPlayerState>(PlayerState);
		if (PartyPlayerState)
		{
			PartyPlayerState->ClientSetPlayerData(NewPlayerData);

			ServerSetPlayerData(NewPlayerData);
			return;
		}
	}

	PartyHost->ProcessPlayerDataChange(this, NewPlayerData);
}

void AKronosPartyClient::ServerSetPlayerData_Implementation(const TArray<int32>& NewPlayerData)
{
	SetPlayerData(NewPlayerData);
}

bool AKronosPartyClient::ServerSetPlayerData_Validate(const TArray<int32>& NewPlayerData)
{
	return true;
}

void AKronosPartyClient::SendChatMessage(const FString& Msg)
{
	if (Msg.IsEmpty())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyClient: Failed to send chat message. Message is empty!"));
		return;
	}

	AKronosPartyHost* PartyHost = Cast<AKronosPartyHost>(GetBeaconOwner());
	if (!PartyHost)
	{
		ServerSendChatMessage(Msg);
		return;
	}

	if (!PlayerState)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyClient: Failed to send chat message. PlayerState is empty!"));
		return;
	}

	PartyHost->ProcessChatMessage(PlayerState->UniqueId, Msg);
}

void AKronosPartyClient::ServerSendChatMessage_Implementation(const FString& Msg)
{
	SendChatMessage(Msg);
}

void AKronosPartyClient::ClientReceiveChatMessage_Implementation(const FUniqueNetIdRepl& SenderId, const FString& Msg)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	PartyManager->OnChatMessageReceived().Broadcast(SenderId, Msg);
}

void AKronosPartyClient::ClientFollowPartyToGameSession_Implementation(const FKronosFollowPartyParams& FollowParams)
{
	FollowPartyParams = FollowParams;
	ClientJoinGame();
}

bool AKronosPartyClient::GetInitialReplicationProps_Implementation() const
{
	return LobbyState != nullptr && PlayerState != nullptr
		&& PlayerState->UniqueId.IsValid() && PlayerState->PartyOwnerUniqueId.IsValid()
		&& PlayerState->ClientActor != nullptr
		&& PlayerState->DisplayName.IsEmpty() != true;
}

void AKronosPartyClient::ClientConnectingToParty()
{
	// Give blueprints a chance to implement custom logic.
	K2_OnConnectingToParty();
}

void AKronosPartyClient::ClientLoginComplete_Implementation(const FUniqueNetIdRepl& InUniqueId, bool bWasSuccessful)
{
	// We are going to hijack the ClientLoginComplete function until initial replication has been completed.
	// The login complete delegate will fire when we call Super.

	if (!bWasSuccessful)
	{
		Super::ClientLoginComplete_Implementation(InUniqueId, bWasSuccessful);
		return;
	}

	FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this, InUniqueId, bWasSuccessful]()
	{
		const bool bReplicationFinished = GetInitialReplicationProps();
		if (bReplicationFinished)
		{
			// Finish the login.
			Super::ClientLoginComplete_Implementation(InUniqueId, bWasSuccessful);

			// This must be called last to avoid timer execution crash.
			GetWorld()->GetTimerManager().ClearTimer(TimerHandle_WaitingInitialReplication);
		}
	});

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_WaitingInitialReplication, TimerDelegate, 0.1f, true, 0.0f);
}

void AKronosPartyClient::OnPartyLoginComplete(bool bWasSuccessful)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyClient: Client login complete with result: %s."), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);

	// In case of a failure, just leave the party.
	// This will make sure that the party session and beacons are destroyed.
	if (!bWasSuccessful)
	{
		// Give blueprints a chance to handle login failure.
		K2_OnPartyLoginComplete(false);

		PartyManager->LeaveParty();
		return;
	}

	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
	if (OnlineSession)
	{
		// Register existing party players. Party host's client beacon do nothing as the host beacon handles this for him.
		if (!PartyManager->IsPartyLeader())
		{
			AKronosPartyState* PartyState = GetPartyState();
			if (PartyState)
			{
				TArray<FUniqueNetIdRepl> PartyPlayers = PartyState->GetPartyPlayerUniqueIds();
				OnlineSession->RegisterPlayers(NAME_PartySession, PartyPlayers);
			}
		}

		// Bind party session updated callback.
		// We'll update the last party info when the session is updated.
		OnlineSession->OnUpdatePartyComplete().AddDynamic(this, &ThisClass::OnPartyUpdated);
	}

	// Update the last party info to reflect this new party that we've just joined or created.
	PartyManager->UpdateLastPartyInfo();

	// Give blueprints a chance to implement custom logic.
	K2_OnPartyLoginComplete(true);

	// Give us a chance to initialize the player client side.
	ClientInitPlayer();

	// Notify the party manager that we connected to the party.
	PartyManager->OnConnectedToParty().Broadcast();
}

void AKronosPartyClient::OnPartyPlayerJoined(const FText& DisplayName, const FUniqueNetIdRepl& UniqueId)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyClient: %s has joined the party."), *DisplayName.ToString());

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);

	// Register the new party player.
	// Party host's client beacon do nothing as the host beacon handles this for him.
	if (!PartyManager->IsPartyLeader())
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
		if (OnlineSession)
		{
			OnlineSession->RegisterPlayer(NAME_PartySession, UniqueId, false);
		}
	}

	PartyManager->UpdateLastPartyInfo();

	// Give blueprints a chance to implement custom logic.
	K2_OnPartyPlayerJoined(DisplayName, UniqueId);

	// Notify the party manager that a player has joined the party.
	PartyManager->OnPlayerJoinedParty().Broadcast(DisplayName, UniqueId);
}

void AKronosPartyClient::OnPartyPlayerLeft(const FUniqueNetIdRepl& UniqueId)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyClient: A player has left the party."));

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);

	// Unregister the leaving party player.
	// Party host's client beacon do nothing as the host beacon handles this for him.
	if (!PartyManager->IsPartyLeader())
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
		if (OnlineSession)
		{
			OnlineSession->UnregisterPlayer(NAME_PartySession, UniqueId);
		}
	}

	PartyManager->UpdateLastPartyInfo();

	// Give blueprints a chance to implement custom logic.
	K2_OnPartyPlayerLeft(UniqueId);

	// Notify the party manager that a player has left the party.
	PartyManager->OnPlayerLeftParty().Broadcast(UniqueId);
}

void AKronosPartyClient::OnPartyUpdated(bool bWasSuccessful)
{
	if (bWasSuccessful && IsLoggedIn())
	{
		UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
		PartyManager->UpdateLastPartyInfo();
	}
}

void AKronosPartyClient::HandleJoiningGame()
{
	JoiningServer();
}

void AKronosPartyClient::HandleJoiningGameAck()
{
	// Give blueprints a chance to cleanup before following the party to the session.
	K2_HandleJoiningGameAck();

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	PartyManager->UpdateLastPartyInfo();

	if (!PartyManager->IsPartyLeader())
	{
		UKronosOnlineSession* KronosOnlineSession = UKronosOnlineSession::Get(this);
		if (KronosOnlineSession)
		{
			KronosOnlineSession->FollowPartyToGameSession(FollowPartyParams);
		}
	}
}

void AKronosPartyClient::ClientWasKicked_Implementation(const FText& KickReason)
{
	Super::ClientWasKicked_Implementation(KickReason);

	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyClient: Client was kicked - Reason: '%s'"), *KickReason.ToString());

	// Give blueprints a chance to implement custom logic.
	K2_ClientWasKicked(KickReason);

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	PartyManager->OnKickedFromParty().Broadcast(KickReason);
}

void AKronosPartyClient::OnFailure()
{
	Super::OnFailure();

	// Give blueprints a chance to implement custom logic.
	K2_OnConnectionFailure();

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	PartyManager->LeaveParty();
}

void AKronosPartyClient::DestroyBeacon()
{
	// Unbind party session update callback.
	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
	if (OnlineSession)
	{
		OnlineSession->OnUpdatePartyComplete().RemoveDynamic(this, &ThisClass::OnPartyUpdated);
	}

	if (GetConnectionState() == EBeaconConnectionState::Pending)
	{
		K2_OnConnectionFailure();
	}

	Super::DestroyBeacon();
}

AKronosPartyState* AKronosPartyClient::GetPartyState() const
{
	if (!LobbyState)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyClient: Failed to get party state. Possibly haven't replicated yet?"));
		return nullptr;
	}

	return Cast<AKronosPartyState>(LobbyState);
}

AKronosPartyPlayerState* AKronosPartyClient::GetPartyPlayerState() const
{
	if (!PlayerState)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyClient: Failed to get party player state. Possibly haven't replicated yet?"));
		return nullptr;
	}

	return Cast<AKronosPartyPlayerState>(PlayerState);
}

int32 AKronosPartyClient::GetPlayerElo() const
{
	AKronosPartyPlayerState* PartyPlayerState = GetPartyPlayerState();
	if (!PartyPlayerState)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyClient: Failed to get player elo."));
		return 0;
	}

	return PartyPlayerState->GetPlayerElo();
}

TArray<int32> AKronosPartyClient::GetPlayerData() const
{
	AKronosPartyPlayerState* PartyPlayerState = GetPartyPlayerState();
	if (!PartyPlayerState)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyClient: Failed to get player data."));
		return TArray<int32>();
	}

	return PartyPlayerState->GetPlayerData();
}

bool AKronosPartyClient::IsLocalPlayer() const
{
	AKronosPartyPlayerState* PartyPlayerState = GetPartyPlayerState();
	if (PartyPlayerState)
	{
		return PartyPlayerState->IsLocalPlayer();
	}

	return false;
}

FString AKronosPartyClient::GetDebugString() const
{
	FString DebugString;

	if (!IsLoggedIn())
	{
		return TEXT("Logging in to party...");
	}

	AKronosPartyState* PartyState = GetPartyState();
	const TArray<AKronosPartyPlayerState*>& PartyPlayerStates = PartyState->GetPartyPlayerStates();

	for (auto It = PartyPlayerStates.CreateConstIterator(); It; ++It)
	{
		AKronosPartyPlayerState* PS = *It;
		DebugString.Appendf(TEXT("%d. %s [%s]\n"), It.GetIndex(), *PS->GetPlayerName().ToString(), PS->IsPartyLeader() ? TEXT("LEADER") : TEXT("CLIENT"));
	}

	return DebugString;
}
