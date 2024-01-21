// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Beacons/KronosPartyHost.h"
#include "Beacons/KronosPartyClient.h"
#include "Beacons/KronosPartyState.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "KronosOnlineSession.h"
#include "KronosUserManager.h"
#include "KronosPartyManager.h"
#include "KronosReservationManager.h"
#include "KronosConfig.h"
#include "Kronos.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AKronosPartyHost::AKronosPartyHost(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ClientBeaconActorClass = GetDefault<UKronosConfig>()->PartyClientClass;
	LobbyStateClass = GetDefault<UKronosConfig>()->PartyStateClass;
}

void AKronosPartyHost::OnInitialized()
{
	K2_OnInitialized();
}

void AKronosPartyHost::ProcessPlayerEloChange(AKronosPartyClient* ClientActor, int32 NewPlayerElo)
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyHost: ProcessPlayerEloChange"));

	if (!ClientActor || !ClientActor->IsLoggedIn())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyHost: ProcessPlayerEloChange - Client is not logged in or the client actor is null."));
		return;
	}

	AKronosPartyPlayerState* PartyPlayerState = Cast<AKronosPartyPlayerState>(ClientActor->PlayerState);
	if (PartyPlayerState)
	{
		// (Not an RPC!) Changes server side player elo that will replicate down to clients.
		PartyPlayerState->ServerSetPlayerElo(NewPlayerElo);
	}
}

void AKronosPartyHost::ProcessPlayerDataChange(AKronosPartyClient* ClientActor, const TArray<int32>& NewPlayerData)
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyHost: ProcessPlayerDataChange"));

	if (!ClientActor || !ClientActor->IsLoggedIn())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyHost: ProcessPlayerDataChange - Client is not logged in or the client actor is null."));
		return;
	}

	AKronosPartyPlayerState* PartyPlayerState = Cast<AKronosPartyPlayerState>(ClientActor->PlayerState);
	if (PartyPlayerState)
	{
		// (Not an RPC!) Changes server side player data that will replicate down to clients.
		PartyPlayerState->ServerSetPlayerData(NewPlayerData);
	}
}

void AKronosPartyHost::ProcessChatMessage(const FUniqueNetIdRepl& SenderId, const FString& Msg)
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyHost: ProcessChatMessage"));

	if (SenderId.IsValid() && !Msg.IsEmpty())
	{
		for (AOnlineBeaconClient* BeaconClient : ClientActors)
		{
			AKronosPartyClient* PartyClient = Cast<AKronosPartyClient>(BeaconClient);
			if (PartyClient && PartyClient->IsLoggedIn())
			{
				PartyClient->ClientReceiveChatMessage(SenderId, Msg);
			}
		}
	}

	else UE_LOG(LogKronos, Error, TEXT("KronosPartyHost: ProcessChatMessage - SenderId is invalid or message is empty."));
}

void AKronosPartyHost::ProcessPartyLeaderMatchmaking(bool bMatchmaking)
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosPartyHost: ProcessPartyLeaderMatchmaking"));

	AKronosPartyState* PartyState = GetPartyState();
	if (PartyState)
	{
		PartyState->ServerSetPartyLeaderMatchmaking(bMatchmaking);
	}
}

bool AKronosPartyHost::ProcessConnectPartyToGameSession()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosPartyHost: Connecting party to game session..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(NAME_GameSession);
			if (!NamedSession)
			{
				UE_LOG(LogKronos, Error, TEXT("KronosPartyHost: No session to connect party to."));
				return false;
			}

			// Make sure that every player is connected properly.
			UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
			if (!PartyManager->IsEveryClientInParty())
			{
				UE_LOG(LogKronos, Error, TEXT("KronosPartyHost: Can't connect party to game session. A player isn't logged in yet, or is already in a game."));
				return false;
			}
			
			// If we are hosting the session, set the host reservation including everyone in the party.
			if (NamedSession->bHosting)
			{
				UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(this);
				FKronosReservation HostReservation = ReservationManager->MakeReservationForParty();
				ReservationManager->SetHostReservation(HostReservation);
			}

			// Tell clients to start following the host to the game session.
			for (AKronosPartyPlayerState* PartyPlayer : PartyManager->GetPartyPlayerStates())
			{
				if (PartyPlayer->ClientActor)
				{
					AKronosPartyClient* PartyClient = Cast<AKronosPartyClient>(PartyPlayer->ClientActor);
					if (PartyClient)
					{
						FKronosFollowPartyParams FollowParams = MakeFollowPartyParamsForClient(PartyPlayer);
						PartyClient->ClientFollowPartyToGameSession(FollowParams);
						PartyClient->ForceNetUpdate();
					}
				}
			}

			GetWorldTimerManager().SetTimer(TimerHandle_ConnectingPartyToGameSession, this, &ThisClass::TickConnectingPartyToGameSession, CONNECT_PARTY_TO_GAMESESSION_TICKRATE, true);
			GetWorldTimerManager().SetTimer(TimerHandle_TimeoutConnectingPartyToGameSession, this, &ThisClass::OnConnectPartyToGameSessionTimeout, CONNECT_PARTY_TO_GAMESESSION_TIMEOUT, false);
			return true;
		}
	}

	return false;
}

FKronosFollowPartyParams AKronosPartyHost::MakeFollowPartyParamsForClient(AKronosPartyPlayerState* InClient)
{
	if (InClient)
	{
		// Since session search results cannot be replicated, each client will somehow have to figure out which session the party host is joining.
		// NOTE: At this point the party host has already joined the desired game session (but haven't connected yet).
		// 
		// We have 3 options:
		// -------------------------
		// 1: If the IOnlineSession::FindFriendSession method is supported, and the client <-> party leader are friends, find the session using the friend's presence data.
		// 2: If the IOnlineSession::FindSessionById method is supported, find the session using its session id.
		// 3: If none of the above is possible, find the session using a regular search with the custom "SETTING_OWNERID" query set.
		//-------------------------
		// This last option has its limitations! E.g. session is private, or session is out of search range.
		// All online subsystems should support finding a specific session in some way, so this last option is most likely only ever used during editor tests with the Null subsystem.

		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem)
		{
			IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
			if (SessionInterface.IsValid())
			{
				FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(NAME_GameSession);
				if (NamedSession)
				{
					// Query via: find friend session.
					if (GetDefault<UKronosConfig>()->bFindFriendSessionSupported)
					{
						UKronosUserManager* UserManager = UKronosUserManager::Get(this);
						if (UserManager->IsFriend(*InClient->UniqueId.GetUniqueNetId(), EFriendsLists::ToString(EFriendsLists::Default)))
						{
							FKronosFollowPartyParams FollowViaFriendIdQuery;
							FollowViaFriendIdQuery.SpecificSessionQuery.Type = EKronosSpecificSessionQueryType::FriendId;
							FollowViaFriendIdQuery.SpecificSessionQuery.UniqueId = InClient->PartyOwnerUniqueId;
							FollowViaFriendIdQuery.bIsLanQuery = NamedSession->SessionSettings.bIsLANMatch;
							FollowViaFriendIdQuery.bSearchPresence = NamedSession->SessionSettings.bUsesPresence;
							FollowViaFriendIdQuery.bPartyLeaderCreatingSession = NamedSession->bHosting;

							return FollowViaFriendIdQuery;
						}
					}

					// Query via: session id.
					if (GetDefault<UKronosConfig>()->bFindSessionByIdSupported)
					{
						FKronosFollowPartyParams FollowViaSessionIdQuery;
						FollowViaSessionIdQuery.SpecificSessionQuery.Type = EKronosSpecificSessionQueryType::SessionId;
						FollowViaSessionIdQuery.SpecificSessionQuery.UniqueId = SessionInterface->CreateSessionIdFromString(NamedSession->GetSessionIdStr());
						FollowViaSessionIdQuery.bIsLanQuery = NamedSession->SessionSettings.bIsLANMatch;
						FollowViaSessionIdQuery.bSearchPresence = NamedSession->SessionSettings.bUsesPresence;
						FollowViaSessionIdQuery.bPartyLeaderCreatingSession = NamedSession->bHosting;

						return FollowViaSessionIdQuery;
					}

					// Query via: session owner id.
					else
					{
						FKronosFollowPartyParams FollowViaSessionOwnerIdQuery;
						FollowViaSessionOwnerIdQuery.SpecificSessionQuery.Type = EKronosSpecificSessionQueryType::SessionOwnerId;
						FollowViaSessionOwnerIdQuery.SpecificSessionQuery.UniqueId = NamedSession->OwningUserId;
						FollowViaSessionOwnerIdQuery.bIsLanQuery = NamedSession->SessionSettings.bIsLANMatch;
						FollowViaSessionOwnerIdQuery.bSearchPresence = NamedSession->SessionSettings.bUsesPresence;
						FollowViaSessionOwnerIdQuery.bPartyLeaderCreatingSession = NamedSession->bHosting;

						return FollowViaSessionOwnerIdQuery;
					}
				}
			}
		}
	}
	
	FKronosFollowPartyParams InvalidQuery;
	return InvalidQuery;
}

AKronosPartyState* AKronosPartyHost::GetPartyState() const
{
	if (!LobbyState)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosPartyClient: Failed to get party state. Party state is null!"));
		return nullptr;
	}

	return Cast<AKronosPartyState>(LobbyState);
}

void AKronosPartyHost::TickConnectingPartyToGameSession()
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	for (AKronosPartyPlayerState* PartyPlayer : PartyManager->GetPartyPlayerStates())
	{
		if (PartyPlayer->bInLobby)
		{
			UE_LOG(LogKronos, Verbose, TEXT("KronosPartyHost: TickConnectingPartyToGameSession() - Not all clients confirmed yet. Waiting..."));
			return;
		}
	}

	OnConnectPartyToGameSessionComplete();
}

void AKronosPartyHost::OnConnectPartyToGameSessionComplete()
{
	UE_LOG(LogKronos, Log, TEXT("KronosPartyHost: Connecting party to game session complete."));

	GetWorldTimerManager().ClearTimer(TimerHandle_ConnectingPartyToGameSession);

	TravelToGameSession();
}

void AKronosPartyHost::OnConnectPartyToGameSessionTimeout()
{
	UE_LOG(LogKronos, Warning, TEXT("KronosPartyHost: Connecting party to game session timed out."));

	GetWorldTimerManager().ClearTimer(TimerHandle_ConnectingPartyToGameSession);

	TravelToGameSession();
}

void AKronosPartyHost::TravelToGameSession()
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	PartyManager->LeavePartyInternal();

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(NAME_GameSession);
			if (NamedSession)
			{
				UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
				if (OnlineSession)
				{
					if (NamedSession->bHosting)
					{
						OnlineSession->ServerTravelToGameSession();
						return;
					}

					else
					{
						OnlineSession->ClientTravelToGameSession();
						return;
					}
				}
			}
		}
	}
}

void AKronosPartyHost::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	// Give blueprints a chance to react to player join immediately, without waiting for the login to complete.
	K2_OnClientJoiningParty();

	Super::OnClientConnected(NewClientActor, ClientConnection);
}

bool AKronosPartyHost::PreLogin(const FUniqueNetIdRepl& InUniqueId, const FString& Options)
{
	// Make sure that the joining player is not banned.
	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
	if (OnlineSession)
	{
		if (OnlineSession->IsPlayerBannedFromSession(NAME_PartySession, *InUniqueId.GetUniqueNetId()))
		{
			// The player is banned from this session.
			return false;
		}
	}

	return true;
}

ALobbyBeaconPlayerState* AKronosPartyHost::HandlePlayerLogin(ALobbyBeaconClient* ClientActor, const FUniqueNetIdRepl& InUniqueId, const FString& Options)
{
	ALobbyBeaconPlayerState* ClientPlayerState = Super::HandlePlayerLogin(ClientActor, InUniqueId, Options);
	if (ClientPlayerState)
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
		if (OnlineSession)
		{
			// Register the player with the session.
			bool bWasFromInvite = UGameplayStatics::HasOption(Options, TEXT("bIsFromInvite"));
			OnlineSession->RegisterPlayer(NAME_PartySession, InUniqueId, bWasFromInvite);
		}

		// Tell the player who the party leader is.
		// Since host migration is not supported, the party leader is always the party host.
		const FUniqueNetIdPtr PartyHostUniqueId = GetWorld()->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl().GetUniqueNetId();
		UpdatePartyLeader(InUniqueId, PartyHostUniqueId);

		// Signal the relevant player state that the party owner changed (server side).
		// This is needed since OnRep functions are not called on the server.
		AKronosPartyPlayerState* PartyPlayerState = Cast<AKronosPartyPlayerState>(ClientPlayerState);
		if (PartyPlayerState)
		{
			PartyPlayerState->SignalPartyOwnerChanged();
		}

		return ClientPlayerState;
	}

	return nullptr;
}

void AKronosPartyHost::PostLogin(ALobbyBeaconClient* ClientActor)
{
	Super::PostLogin(ClientActor);

	AKronosPartyClient* PartyClientActor = CastChecked<AKronosPartyClient>(ClientActor);
	if (PartyClientActor)
	{
		// Give us a chance to initialize the player on server side.
		PartyClientActor->ServerInitPlayer();

		// Give blueprints a chance to implement custom logic.
		K2_OnClientJoinedParty(PartyClientActor);
	}
}

void AKronosPartyHost::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	AKronosPartyClient* PartyClientActor = CastChecked<AKronosPartyClient>(LeavingClientActor);
	if (PartyClientActor)
	{
		// Give blueprints a chance to implement custom logic.
		K2_OnClientLeavingParty(PartyClientActor);
	}

	//~ Begin Super::NotifyClientDisconnected Implementation
	if (LobbyState)
	{
		ALobbyBeaconPlayerState* Player = LobbyState->GetPlayer(LeavingClientActor);
		if (Player && Player->bInLobby)
		{
			UWorld* World = GetWorld();
			check(World);

			AGameModeBase* GameMode = World->GetAuthGameMode();
			check(GameMode);
			check(GameMode->GameSession);

			// Override the session name to NAME_PartySession.
			// By default the lobby beacon would attempt to unregister the player from NAME_GameSession.
			GameMode->GameSession->NotifyLogout(NAME_PartySession, Player->UniqueId);
			HandlePlayerLogout(Player->UniqueId);
		}
	}
	else
	{
		UE_LOG(LogLobbyBeacon, Warning, TEXT("No lobby beacon state to handle disconnection!"));
	}
	//~ End Super::NotifyClientDisconnected Implementation

	// Notice that we are not calling Super.
	AOnlineBeaconHostObject::NotifyClientDisconnected(LeavingClientActor);
}
