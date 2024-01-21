// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosPartyManager.h"
#include "KronosMatchmakingManager.h"
#include "KronosOnlineSession.h"
#include "Beacons/KronosPartyListener.h"
#include "Beacons/KronosPartyHost.h"
#include "Beacons/KronosPartyClient.h"
#include "Beacons/KronosPartyState.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "KronosConfig.h"
#include "Kronos.h"
#include "LobbyBeaconPlayerState.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

UKronosPartyManager* UKronosPartyManager::Get(const UObject* WorldContextObject)
{
	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
	if (OnlineSession)
	{
		return OnlineSession->GetPartyManager();
	}

	return nullptr;
}

bool UKronosPartyManager::InitPartyBeaconHost(const int32 MaxNumPlayers)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyManager: Creating party beacon host..."));

	// Create party listener.
	PartyBeaconListener = GetWorld()->SpawnActor<AKronosPartyListener>(GetDefault<UKronosConfig>()->PartyListenerClass);
	if (PartyBeaconListener)
	{
		if (PartyBeaconListener->InitHost())
		{
			UE_LOG(LogKronos, Verbose, TEXT("Beacon host listener initialized."));

			// Create party host.
			PartyBeaconHost = GetWorld()->SpawnActor<AKronosPartyHost>(GetDefault<UKronosConfig>()->PartyHostClass);
			if (PartyBeaconHost)
			{
				if (PartyBeaconHost->Init(NAME_PartySession))
				{
					UE_LOG(LogKronos, Verbose, TEXT("Beacon host initialized."));

					PartyBeaconListener->RegisterHost(PartyBeaconHost);
					PartyBeaconHost->SetupLobbyState(MaxNumPlayers);
					PartyBeaconListener->PauseBeaconRequests(false);

					PartyBeaconHost->OnInitialized();

					// Create a client for the party host.
					if (InitPartyBeaconClientForHost())
					{
						KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyManager: Party created!"));
						return true;
					}
				}
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPartyManager: InitPartyBeaconHost failed."));
	return false;
}

bool UKronosPartyManager::InitPartyBeaconClientForHost()
{
	UE_LOG(LogKronos, Log, TEXT("KronosPartyManager: Creating party beacon client for host..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			PartyBeaconClient = GetWorld()->SpawnActor<AKronosPartyClient>(GetDefault<UKronosConfig>()->PartyClientClass);
			if (PartyBeaconClient)
			{
				FNamedOnlineSession* PartySession = SessionInterface->GetNamedSession(NAME_PartySession);
				if (PartySession)
				{
					PartyBeaconClient->SetDestSessionId(PartySession->GetSessionIdStr());
					PartyBeaconClient->SetBeaconOwner(PartyBeaconHost);

					PartyBeaconClient->OnConnected();

					// We have to check if the party client is valid after OnConnected() because in PIE the login will fail, and the client will receive a network error.
					// Once the network error is received the client will initiate the leave party process immediately, destroying all party beacons and leaving the party session.
					if (IsValid(PartyBeaconClient))
					{
						PartyBeaconHost->OnClientConnected(PartyBeaconClient, nullptr);

						UE_LOG(LogKronos, Log, TEXT("KronosPartyManager: Beacon client initialized for party host."));
						return true;
					}
				}
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPartyManager: InitPartyBeaconClientForHost failed."));
	return false;
}

bool UKronosPartyManager::InitPartyBeaconClient()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyManager: Creating party beacon client..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FNamedOnlineSession* PartySession = SessionInterface->GetNamedSession(NAME_PartySession);
			if (PartySession)
			{
				FString ConnectString;
				if (SessionInterface->GetResolvedConnectString(NAME_PartySession, ConnectString, NAME_BeaconPort))
				{
					PartyBeaconClient = GetWorld()->SpawnActor<AKronosPartyClient>(GetDefault<UKronosConfig>()->PartyClientClass);
					if (PartyBeaconClient)
					{
						FURL ConnectURL = FURL(nullptr, *ConnectString, TRAVEL_Absolute);
						if (PartyBeaconClient->InitClient(ConnectURL))
						{
							KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyManager: Beacon client initialized. Connecting..."));

							PartyBeaconClient->SetDestSessionId(PartySession->GetSessionIdStr());
							PartyBeaconClient->ClientConnectingToParty();
							return true;
						}
					}
				}
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPartyManager: InitPartyBeaconClient failed."));
	return false;
}

void UKronosPartyManager::LeaveParty(const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	// User requested leave party with no intention of reconnecting later
	// so we'll clear the last party information now.
	ClearLastPartyInfo();
	LeavePartyInternal(CompletionDelegate);
}

void UKronosPartyManager::LeavePartyInternal(const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyManager: Leaving party..."));

	// Disconnect from the party.
	DestroyPartyBeacons();

	// Leave the session.
	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
	if (OnlineSession)
	{
		KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Destroying PartySession..."));
		OnlineSession->DestroySession(NAME_PartySession, CompletionDelegate);
	}
}

void UKronosPartyManager::DestroyPartyBeacons()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyManager: Destroying party beacons..."));

	// Clean up party client beacon.
	if (PartyBeaconClient)
	{
		// Clean up party state.
		// Note that on client side the party state may already be in pending kill state due to the net driver destroying it.
		AKronosPartyState* PartyState = PartyBeaconClient->GetPartyState();
		if (IsValid(PartyState))
		{
			const bool bNetForce = true;
			PartyState->Destroy(bNetForce);
		}

		PartyBeaconClient->DestroyBeacon();
		PartyBeaconClient = nullptr;
	}

	// Clean up party host beacon.
	if (PartyBeaconHost)
	{
		PartyBeaconHost->Destroy();
		PartyBeaconHost = nullptr;
	}
	
	// Clean up party listener.
	if (PartyBeaconListener)
	{
		PartyBeaconListener->DestroyBeacon();
		PartyBeaconListener = nullptr;
	}

	OnDisconnectedFromPartyEvent.Broadcast();
}

void UKronosPartyManager::KickPlayerFromParty(const FUniqueNetIdRepl& UniqueId, const FText& Reason, bool bBanPlayerFromSession)
{
	if (PartyBeaconClient && IsPartyLeader())
	{
		PartyBeaconClient->KickPlayer(UniqueId, Reason);

		if (bBanPlayerFromSession)
		{
			UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
			if (OnlineSession)
			{
				OnlineSession->BanPlayerFromSession(NAME_PartySession, *UniqueId.GetUniqueNetId());
			}
		}
	}
}

void UKronosPartyManager::SetPartyLeaderMatchmaking(bool bMatchmaking)
{
	if (IsPartyLeader())
	{
		PartyBeaconHost->ProcessPartyLeaderMatchmaking(bMatchmaking);
	}
}

void UKronosPartyManager::UpdateLastPartyInfo()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FNamedOnlineSession* NamedPartySession = SessionInterface->GetNamedSession(NAME_PartySession);
			if (NamedPartySession)
			{
				AKronosPartyPlayerState* LocalPlayerState = GetClientBeacon()->GetPartyPlayerState();
				if (LocalPlayerState)
				{
					LastPartyInfo.LastPartyRole = LocalPlayerState->IsPartyLeader() ? EKronosPartyRole::PartyHost : EKronosPartyRole::PartyClient;
					LastPartyInfo.LastPartyHostPlayerId = LocalPlayerState->GetPartyLeaderId();
				}

				LastPartyInfo.LastPartySessionId = NamedPartySession->GetSessionIdStr();
				LastPartyInfo.LastPartyPlayerCount = NamedPartySession->SessionSettings.NumPublicConnections - NamedPartySession->NumOpenPublicConnections;
				LastPartyInfo.LastPartySettings = NamedPartySession->SessionSettings;
			}
		}
	}
}

bool UKronosPartyManager::IsInParty() const
{
	return PartyBeaconClient && PartyBeaconClient->IsLoggedIn();
}

bool UKronosPartyManager::IsPartyLeader() const
{
	return PartyBeaconHost != nullptr;
}

bool UKronosPartyManager::IsEveryClientInParty() const
{
	for (AKronosPartyPlayerState* PartyPlayer : GetPartyPlayerStates())
	{
		if (!PartyPlayer->bInLobby)
		{
			return false;
		}
	}

	return true;
}

bool UKronosPartyManager::GetPartyLeaderUniqueId(FUniqueNetIdRepl& OutUniqueId) const
{
	if (IsInParty())
	{
		if (PartyBeaconClient->PlayerState)
		{
			if (PartyBeaconClient->PlayerState->PartyOwnerUniqueId.IsValid())
			{
				OutUniqueId = PartyBeaconClient->PlayerState->PartyOwnerUniqueId;
				return true;
			}
			
			UE_LOG(LogKronos, Error, TEXT("KronosPartyManager: Failed to get party leader unique id. Unique id was invalid!"));
			return false;
		}

		UE_LOG(LogKronos, Error, TEXT("KronosPartyManager: Failed to get party leader unique id. Client beacon PlayerState was invalid! (Possibly haven't replicated yet.)"));
		return false;
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPartyManager: Failed to get party leader unique id. Player is not in party."));
	return false;
}

bool UKronosPartyManager::IsPartyLeaderMatchmaking() const
{
	if (IsInParty())
	{
		AKronosPartyState* PartyState = GetPartyState();
		if (PartyState)
		{
			return PartyState->IsPartyLeaderMatchmaking();
		}
	}

	return false;
}

int32 UKronosPartyManager::GetPartySize() const
{
	if (IsInParty())
	{
		return GetNumPlayersInParty();
	}
	
	return 1;
}

int32 UKronosPartyManager::GetNumPlayersInParty() const
{
	if (IsInParty())
	{
		if (PartyBeaconClient->LobbyState)
		{
			return PartyBeaconClient->LobbyState->GetNumPlayers();
		}

		else UE_LOG(LogKronos, Error, TEXT("KronosPartyManager: Failed to get num players in party. Client beacon LobbyState was invalid! (Possibly haven't replicated yet.)"));
	}

	return 0;
}

int32 UKronosPartyManager::GetMaxNumPlayersInParty() const
{
	if (IsInParty())
	{
		if (PartyBeaconClient->LobbyState)
		{
			return PartyBeaconClient->LobbyState->GetMaxPlayers();
		}

		else UE_LOG(LogKronos, Error, TEXT("KronosPartyManager: Failed to get max num players in party. Client beacon LobbyState was invalid! (Possibly haven't replicated yet.)"));
	}

	return 0;
}

int32 UKronosPartyManager::GetPartyEloAverage() const
{
	if (IsInParty())
	{
		AKronosPartyState* PartyState = PartyBeaconClient->GetPartyState();
		if (PartyState)
		{
			return PartyState->GetPartyEloAverage();
		}
	}

	return 0;
}

AKronosPartyState* UKronosPartyManager::GetPartyState() const
{
	if (IsInParty())
	{
		return PartyBeaconClient->GetPartyState();
	}

	return nullptr;
}

AKronosPartyClient* UKronosPartyManager::GetPartyClient(const FUniqueNetIdRepl& UniqueId)
{
	if (IsInParty())
	{
		AKronosPartyState* PartyState = GetPartyState();
		if (PartyState)
		{
			return PartyState->GetPartyClient(UniqueId);
		}
	}

	return nullptr;
}

AKronosPartyPlayerState* UKronosPartyManager::GetPartyPlayerState(const FUniqueNetIdRepl& UniqueId)
{
	if (IsInParty())
	{
		AKronosPartyState* PartyState = GetPartyState();
		if (PartyState)
		{
			return PartyState->GetPartyPlayerState(UniqueId);
		}
	}

	return nullptr;
}

TArray<AKronosPartyPlayerState*> UKronosPartyManager::GetPartyPlayerStates() const
{
	if (IsInParty())
	{
		AKronosPartyState* PartyState = GetPartyState();
		if (PartyState)
		{
			return PartyState->GetPartyPlayerStates();
		}
	}

	return TArray<AKronosPartyPlayerState*>();
}

TArray<FUniqueNetIdRepl> UKronosPartyManager::GetPartyPlayerUniqueIds() const
{
	if (IsInParty())
	{
		AKronosPartyState* PartyState = GetPartyState();
		if (PartyState)
		{
			return PartyState->GetPartyPlayerUniqueIds();
		}
	}

	return TArray<FUniqueNetIdRepl>();
}

void UKronosPartyManager::DumpPartyState() const
{
	UE_LOG(LogKronos, Log, TEXT("Dumping party state..."));

	if (!IsInParty())
	{
		UE_LOG(LogKronos, Log, TEXT("  Player is not in a party."));
		return;
	}

	AKronosPartyState* PartyState = GetPartyState();
	const TArray<AKronosPartyPlayerState*>& PartyPlayerStates = PartyState->GetPartyPlayerStates();

	for (auto It = PartyPlayerStates.CreateConstIterator(); It; ++It)
	{
		AKronosPartyPlayerState* PlayerState = *It;
		UE_LOG(LogKronos, Log, TEXT("  %d. %s [%s]"), It.GetIndex(), *PlayerState->GetPlayerName().ToString(), PlayerState->IsPartyLeader() ? TEXT("LEADER") : TEXT("CLIENT"));
	}
}

UWorld* UKronosPartyManager::GetWorld() const
{
	// We don't care about the CDO because it's not relevant to world checks.
	// If we don't return nullptr here then blueprints are going to complain.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	return GetOuter()->GetWorld();
}
