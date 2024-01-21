// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosStatics.h"
#include "KronosOnlineSession.h"
#include "KronosUserManager.h"
#include "KronosMatchmakingManager.h"
#include "KronosPartyManager.h"
#include "KronosReservationManager.h"
#include "KronosConfig.h"
#include "Kronos.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Engine/Engine.h"

bool UKronosStatics::IsAuthenticated(const UObject* WorldContextObject)
{
	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	return UserManager->IsAuthenticated();
}

bool UKronosStatics::IsLoggedIn(const UObject* WorldContextObject)
{
	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	return UserManager->IsLoggedIn();
}

FUniqueNetIdRepl UKronosStatics::GetLocalPlayerId(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		return World->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl();
	}

	static FUniqueNetIdRepl EmptyId = FUniqueNetIdRepl();
	return EmptyId;
}

FString UKronosStatics::GetPlayerNickname(const UObject* WorldContextObject)
{
	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	return UserManager->GetUserNickname();
}

bool UKronosStatics::IsMatchmaking(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
		return MatchmakingManager->IsMatchmaking();
	}

	return false;
}

EKronosMatchmakingState UKronosStatics::GetMatchmakingState(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
		return MatchmakingManager->GetMatchmakingState();
	}

	return EKronosMatchmakingState::NotStarted;
}

EKronosMatchmakingCompleteResult UKronosStatics::GetMatchmakingResult(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
		return MatchmakingManager->GetMatchmakingResult();
	}

	return EKronosMatchmakingCompleteResult::Failure;
}

EKronosMatchmakingFailureReason UKronosStatics::GetMatchmakingFailureReason(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
		return MatchmakingManager->GetMatchmakingFailureReason();
	}

	return EKronosMatchmakingFailureReason::Unknown;
}

TArray<FKronosSearchResult> UKronosStatics::GetMatchmakingSearchResults(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
		return MatchmakingManager->GetMatchmakingSearchResults();
	}

	return TArray<FKronosSearchResult>();
}

void UKronosStatics::ServerTravelToLevel(const UObject* WorldContextObject, FString TravelURL)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogKronos, Error, TEXT("KronosStatics: ServerTravelToLevel was called client side. Only the server has authority to ServerTravel."));
			return;
		}

		if (World->GetNetMode() == NM_Standalone)
		{
			UE_LOG(LogKronos, Warning, TEXT("KronosStatics: ServerTravelToLevel was called in a standalone environment. Consider using OpenLevel instead."));
		}

		World->ServerTravel(TravelURL);
	}
}

void UKronosStatics::StartMatch(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogKronos, Error, TEXT("KronosStatics: StartMatch was called client side. Only the server has authority to start matches."));
			return;
		}

		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (GameMode)
		{
			AGameSession* GameSession = GameMode->GameSession;
			if (GameSession)
			{
				GameSession->HandleMatchHasStarted();
			}
		}
	}
}

void UKronosStatics::EndMatch(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogKronos, Error, TEXT("KronosStatics: EndMatch was called client side. Only the server has authority to end matches."));
			return;
		}

		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (GameMode)
		{
			AGameSession* GameSession = GameMode->GameSession;
			if (GameSession)
			{
				GameSession->HandleMatchHasEnded();
			}
		}
	}
}

bool UKronosStatics::KickPlayerFromMatch(const UObject* WorldContextObject, APlayerController* KickedPlayer, bool bBanFromSession)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogKronos, Error, TEXT("KronosStatics: KickPlayerFromMatch was called client side. Only the server has authority to kick players from matches."));
			return false;
		}

		AGameModeBase* GameMode = World->GetAuthGameMode();
		if (GameMode)
		{
			AGameSession* GameSession = GameMode->GameSession;
			if (GameSession)
			{
				// Cache unique id of the player before kicking in case we also want to ban.
				const FUniqueNetIdRepl PlayerId = KickedPlayer->PlayerState->GetUniqueId();

				if (GameSession->KickPlayer(KickedPlayer, FText::GetEmpty()))
				{
					if (bBanFromSession)
					{
						BanPlayerFromMatch(WorldContextObject, PlayerId);
					}

					return true;
				}
			}
		}
	}

	return false;
}

bool UKronosStatics::BanPlayerFromMatch(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogKronos, Error, TEXT("KronosStatics: BanPlayerFromMatch was called client side. Only the server has authority to ban players from matches."));
			return false;
		}

		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->BanPlayerFromSession(NAME_GameSession, *PlayerId.GetUniqueNetId());
		}
	}

	return false;
}

bool UKronosStatics::IsPlayerBannedFromMatch(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->IsPlayerBannedFromSession(NAME_GameSession, *PlayerId.GetUniqueNetId());
		}
	}

	return false;
}

bool UKronosStatics::SetHostReservation(const UObject* WorldContextObject, const FKronosReservation& Reservation)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->SetHostReservation(Reservation);
}

bool UKronosStatics::SetHostReservations(const UObject* WorldContextObject, const TArray<FKronosReservation>& Reservations)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->SetHostReservations(Reservations);
}

TArray<FKronosReservation> UKronosStatics::CopyRegisteredReservations(const UObject* WorldContextObject, bool bCleanupReservations)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->CopyRegisteredReservations(bCleanupReservations);
}

bool UKronosStatics::ReconfigureMaxReservations(const UObject* WorldContextObject, const int32 MaxReservations)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->ReconfigureMaxReservations(MaxReservations);
}

bool UKronosStatics::CompleteReservation(const UObject* WorldContextObject, const APlayerController* PlayerController)
{
	if (PlayerController && PlayerController->PlayerState)
	{
		UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
		return ReservationManager->CompleteReservation(PlayerController->PlayerState->GetUniqueId());
	}

	return false;
}

bool UKronosStatics::CompleteReservationById(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->CompleteReservation(PlayerId);
}

bool UKronosStatics::IsReservationHost(const UObject* WorldContextObject)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->IsReservationHost();
}

bool UKronosStatics::PlayerHasReservation(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->PlayerHasReservation(PlayerId);
}

AKronosReservationHost* UKronosStatics::GetReservationHost(const UObject* WorldContextObject)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->GetHostBeacon();
}

FKronosReservation UKronosStatics::MakeReservationForPrimaryPlayer(const UObject* WorldContextObject)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->MakeReservationForPrimaryPlayer();
}

FKronosReservation UKronosStatics::MakeReservationForParty(const UObject* WorldContextObject)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->MakeReservationForParty();
}

FKronosReservation UKronosStatics::MakeReservationForGamePlayers(const UObject* WorldContextObject)
{
	UKronosReservationManager* ReservationManager = UKronosReservationManager::Get(WorldContextObject);
	return ReservationManager->MakeReservationForGamePlayers();
}

void UKronosStatics::KickPlayerFromParty(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId, bool bBanFromSession)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	PartyManager->KickPlayerFromParty(PlayerId, FText::GetEmpty(), bBanFromSession);
}

bool UKronosStatics::BanPlayerFromParty(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (IsPartyLeader(WorldContextObject))
		{
			UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
			if (OnlineSession)
			{
				return OnlineSession->BanPlayerFromSession(NAME_PartySession, *PlayerId.GetUniqueNetId());
			}
		}
	}

	return false;
}

bool UKronosStatics::IsInParty(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->IsInParty();
}

bool UKronosStatics::IsPartyLeader(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->IsPartyLeader();
}

bool UKronosStatics::IsEveryClientInParty(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->IsEveryClientInParty();
}

bool UKronosStatics::IsPlayerBannedFromParty(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->IsPlayerBannedFromSession(NAME_PartySession, *PlayerId.GetUniqueNetId());
		}
	}

	return false;
}

bool UKronosStatics::IsPartyLeaderMatchmaking(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->IsPartyLeaderMatchmaking();
}

int32 UKronosStatics::GetPartySize(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->GetPartySize();
}

int32 UKronosStatics::GetPartyEloAverage(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->GetPartyEloAverage();
}

int32 UKronosStatics::GetNumPlayersInParty(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->GetNumPlayersInParty();
}

int32 UKronosStatics::GetMaxNumPlayersInParty(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->GetMaxNumPlayersInParty();
}

AKronosPartyHost* UKronosStatics::GetPartyHost(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->GetHostBeacon();
}

AKronosPartyClient* UKronosStatics::GetPartyClient(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	if (PartyManager->IsInParty())
	{
		return PartyManager->GetPartyClient(PlayerId);
	}

	return nullptr;
}

AKronosPartyState* UKronosStatics::GetPartyState(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->GetPartyState();
}

AKronosPartyPlayerState* UKronosStatics::GetPartyPlayerState(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	if (PartyManager->IsInParty())
	{
		return PartyManager->GetPartyPlayerState(PlayerId);
	}

	return nullptr;
}

TArray<AKronosPartyPlayerState*> UKronosStatics::GetPartyPlayerStates(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	if (PartyManager->IsInParty())
	{
		return PartyManager->GetPartyPlayerStates();
	}

	return TArray<AKronosPartyPlayerState*>();
}

TArray<FUniqueNetIdRepl> UKronosStatics::GetPartyPlayerUniqueIds(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	if (PartyManager->IsInParty())
	{
		return PartyManager->GetPartyPlayerUniqueIds();
	}

	return TArray<FUniqueNetIdRepl>();
}

bool UKronosStatics::HasLastPartyInfo(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->GetLastPartyInfo().IsValid();
}

FKronosLastPartyInfo UKronosStatics::GetLastPartyInfo(const UObject* WorldContextObject)
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	return PartyManager->GetLastPartyInfo();
}

FKronosOnlineFriend UKronosStatics::GetFriend(const UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, const FString& ListName)
{
	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	return UserManager->GetFriend(*FriendId, ListName);
}

int32 UKronosStatics::GetFriendCount(const UObject* WorldContextObject, bool bInGamePlayersOnly) const
{
	FString FriendsListToRead = EFriendsLists::ToString(EFriendsLists::OnlinePlayers);
	if (bInGamePlayersOnly)
	{
		FriendsListToRead = EFriendsLists::ToString(EFriendsLists::InGamePlayers);
	}

	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	return UserManager->GetFriendCount(FriendsListToRead);
}

bool UKronosStatics::SendGameInviteToFriend(const UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId)
{
	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	return UserManager->SendSessionInviteToFriend(NAME_GameSession, *FriendId.GetUniqueNetId());
}

bool UKronosStatics::SendPartyInviteToFriend(const UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId)
{
	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	return UserManager->SendSessionInviteToFriend(NAME_PartySession, *FriendId.GetUniqueNetId());
}

bool UKronosStatics::IsFriend(const UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, const FString& ListName)
{
	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	return UserManager->IsFriend(*FriendId.GetUniqueNetId(), ListName);
}

bool UKronosStatics::ShowGameInviteUI()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineExternalUIPtr ExternalUIInterface = OnlineSubsystem->GetExternalUIInterface();
		if (ExternalUIInterface.IsValid())
		{
			return ExternalUIInterface->ShowInviteUI(0, NAME_GameSession);
		}
	}

	return false;
}

bool UKronosStatics::ShowPartyInviteUI()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineExternalUIPtr ExternalUIInterface = OnlineSubsystem->GetExternalUIInterface();
		if (ExternalUIInterface.IsValid())
		{
			return ExternalUIInterface->ShowInviteUI(0, NAME_PartySession);
		}
	}

	return false;
}

bool UKronosStatics::ShowProfileUI(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem)
		{
			IOnlineExternalUIPtr ExternalUIInterface = OnlineSubsystem->GetExternalUIInterface();
			if (ExternalUIInterface.IsValid())
			{
				FUniqueNetIdPtr Requestor = World->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl().GetUniqueNetId();
				FUniqueNetIdPtr Requestee = PlayerId.GetUniqueNetId();

				return ExternalUIInterface->ShowProfileUI(*Requestor, *Requestee);
			}
		}
	}

	return false;
}

bool UKronosStatics::GetGameSessionSettings(const UObject* WorldContextObject, FKronosSessionSettings& SessionSettings)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSettings(NAME_GameSession, SessionSettings);
		}
	}

	return false;
}

bool UKronosStatics::GetPartySessionSettings(const UObject* WorldContextObject, FKronosSessionSettings& SessionSettings)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSettings(NAME_PartySession, SessionSettings);
		}
	}

	return false;
}

bool UKronosStatics::GetGameSessionSettingInt32(const UObject* WorldContextObject, FName Key, int32& Value)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSetting(NAME_GameSession, Key, Value);
		}
	}

	return false;
}

bool UKronosStatics::GetGameSessionSettingFString(const UObject* WorldContextObject, FName Key, FString& Value)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSetting(NAME_GameSession, Key, Value);
		}
	}

	return false;
}

bool UKronosStatics::GetGameSessionSettingFloat(const UObject* WorldContextObject, FName Key, float& Value)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSetting(NAME_GameSession, Key, Value);
		}
	}

	return false;
}

bool UKronosStatics::GetGameSessionSettingBool(const UObject* WorldContextObject, FName Key, bool& Value)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSetting(NAME_GameSession, Key, Value);
		}
	}

	return false;
}

bool UKronosStatics::GetPartySessionSettingInt32(const UObject* WorldContextObject, FName Key, int32& Value)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSetting(NAME_PartySession, Key, Value);
		}
	}

	return false;
}

bool UKronosStatics::GetPartySessionSettingFString(const UObject* WorldContextObject, FName Key, FString& Value)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSetting(NAME_PartySession, Key, Value);
		}
	}

	return false;
}

bool UKronosStatics::GetPartySessionSettingFloat(const UObject* WorldContextObject, FName Key, float& Value)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSetting(NAME_PartySession, Key, Value);
		}
	}

	return false;
}

bool UKronosStatics::GetPartySessionSettingBool(const UObject* WorldContextObject, FName Key, bool& Value)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
		if (OnlineSession)
		{
			return OnlineSession->GetSessionSetting(NAME_PartySession, Key, Value);
		}
	}

	return false;
}

FUniqueNetIdRepl UKronosStatics::GetSessionUniqueId(const FKronosSearchResult& SearchResult)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			return SessionInterface->CreateSessionIdFromString(SearchResult.OnlineResult.GetSessionIdStr());
		}
	}

	static FUniqueNetIdRepl EmptyId = FUniqueNetIdRepl();
	return EmptyId;
}

int32 UKronosStatics::MakeKronosMatchmakingFlags(const bool bNoHost, const bool bSkipReservation, const bool bSkipEloChecks)
{
	uint8 Result = 0;
	if (bNoHost) Result |= static_cast<uint8>(EKronosMatchmakingFlags::NoHost);
	if (bSkipReservation) Result |= static_cast<uint8>(EKronosMatchmakingFlags::SkipReservation);
	if (bSkipEloChecks) Result |= static_cast<uint8>(EKronosMatchmakingFlags::SkipEloChecks);

	return Result;
}

bool UKronosStatics::IsTearingDownWorld(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		return World->bIsTearingDown;
	}

	return false;
}

const FUniqueNetIdRepl& UKronosStatics::GetPlayerUniqueId(const APlayerState* Player)
{
	if (Player)
	{
		return Player->GetUniqueId();
	}

	static FUniqueNetIdRepl EmptyId = FUniqueNetIdRepl();
	return EmptyId;
}

APlayerState* UKronosStatics::GetPlayerStateFromUniqueId(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (!PlayerId.IsValid())
		{
			UE_LOG(LogKronos, Error, TEXT("KronosStatics: GetPlayerStateFromUniqueId called with invalid player id."));
			return nullptr;
		}

		AGameStateBase* GameState = World->GetGameState();
		if (GameState)
		{
			for (APlayerState* PlayerState : GameState->PlayerArray)
			{
				if (PlayerState->GetUniqueId() == PlayerId)
				{
					return PlayerState;
				}
			}
		}
	}

	return nullptr;
}

APlayerController* UKronosStatics::GetPlayerControllerFromUniqueId(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId)
{
	if (!PlayerId.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosStatics: GetPlayerControllerFromUniqueId called with invalid player id."));
		return nullptr;
	}

	APlayerState* PlayerState = GetPlayerStateFromUniqueId(WorldContextObject, PlayerId);
	if (PlayerState)
	{
		return PlayerState->GetOwner<APlayerController>();
	}

	return nullptr;
}
