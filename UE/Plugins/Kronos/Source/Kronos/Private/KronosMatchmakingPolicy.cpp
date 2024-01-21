// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingSearchPass.h"
#include "KronosOnlineSession.h"
#include "KronosPartyManager.h"
#include "KronosStatics.h"
#include "KronosConfig.h"
#include "Kronos.h"
#include "Beacons/KronosReservationClient.h"
#include "OnlineSubsystem.h"
#include "TimerManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"

UKronosMatchmakingPolicy::UKronosMatchmakingPolicy(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete);
		OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete);
	}
}

void UKronosMatchmakingPolicy::StartMatchmaking(const FName InSessionName, const FKronosMatchmakingParams& InParams, const uint8 InFlags, const EKronosMatchmakingMode InMode, const float InStartDelay, const FKronosSearchResult& InSessionToJoin)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Starting matchmaking..."));

	if (bWasStarted)
	{
		UE_LOG(LogKronos, Warning, TEXT("This matchmaking policy has already been started once. To restart matchmaking with different params, create a new matchmaking policy."));
		UE_LOG(LogKronos, Error, TEXT("Failed to start matchmaking!"));

		SignalMatchmakingComplete(EKronosMatchmakingState::NotStarted, EKronosMatchmakingCompleteResult::Failure, EKronosMatchmakingFailureReason::InvalidParams);
		return;
	}

	if (InSessionName != NAME_GameSession && InSessionName != NAME_PartySession)
	{
		UE_LOG(LogKronos, Warning, TEXT("SessionName is invalid. Make sure to use either 'NAME_GameSession' or 'NAME_PartySession'."));
		UE_LOG(LogKronos, Error, TEXT("Failed to start matchmaking!"));

		SignalMatchmakingComplete(EKronosMatchmakingState::NotStarted, EKronosMatchmakingCompleteResult::Failure, EKronosMatchmakingFailureReason::InvalidParams);
		return;
	}

	if (!InParams.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("Failed to start matchmaking!"));

		SignalMatchmakingComplete(EKronosMatchmakingState::NotStarted, EKronosMatchmakingCompleteResult::Failure, EKronosMatchmakingFailureReason::InvalidParams);
		return;
	}

	if (InSessionName == NAME_GameSession)
	{
		const bool bMatchmakingCanResultInHosting = ((InMode == EKronosMatchmakingMode::Default || InMode == EKronosMatchmakingMode::CreateOnly));
		const bool bCanBecomeHost = !(InFlags & static_cast<uint8>(EKronosMatchmakingFlags::NoHost));

		if (bMatchmakingCanResultInHosting && bCanBecomeHost)
		{
			if (!InParams.HostParams.IsValid())
			{
				UE_LOG(LogKronos, Error, TEXT("Failed to start matchmaking!"));

				SignalMatchmakingComplete(EKronosMatchmakingState::NotStarted, EKronosMatchmakingCompleteResult::Failure, EKronosMatchmakingFailureReason::InvalidParams);
				return;
			}
		}
	}

	SearchPass = NewObject<UKronosMatchmakingSearchPass>(this, GetSearchPassClass());
	SearchPass->OnSearchPassComplete().BindUObject(this, &ThisClass::OnSearchPassComplete);
	SearchPass->OnCancelSearchPassComplete().BindUObject(this, &ThisClass::OnCancelSearchPassComplete);

	SessionName = InSessionName;
	MatchmakingParams = InParams;
	MatchmakingFlags = InFlags;
	MatchmakingMode = InMode;
	SessionToJoin = InSessionToJoin;

	if (UE_LOG_ACTIVE(LogKronos, Verbose))
	{
		DumpSettings();
	}

	bWasStarted = true;
	bMatchmakingInProgress = true;

	MatchmakingTime = 0;
	CurrentMatchmakingPassIdx = 1;

	FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this]()
	{
		MatchmakingTime++;
		OnKronosMatchmakingUpdated().Broadcast(MatchmakingState, MatchmakingTime);
	});

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_MatchmakingTimer, TimerDelegate, 1.0f, true);

	SignalStartMatchmakingComplete();
	UE_LOG(LogKronos, Log, TEXT("Matchmaking attempt: %d/%d"), CurrentMatchmakingPassIdx, MatchmakingParams.MaxSearchAttempts);

	if (InStartDelay > 0.0f)
	{
		SetMatchmakingState(EKronosMatchmakingState::Starting);

		FTimerDelegate CompletionDelegate = FTimerDelegate::CreateLambda([this]()
		{
			BeginMatchmaking();
		});

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_MatchmakingDelay, CompletionDelegate, InStartDelay, false);
		return;
	}

	else
	{
		BeginMatchmaking();
		return;
	}
}

void UKronosMatchmakingPolicy::CancelMatchmaking()
{
	const bool bIsMatchmaking = bMatchmakingInProgress && MatchmakingState != EKronosMatchmakingState::Canceled;
	if (!bIsMatchmaking)
	{
		UE_LOG(LogKronos, Warning, TEXT("There is no matchmaking to cancel."));
		return;
	}
	
	if (MatchmakingState == EKronosMatchmakingState::Canceling)
	{
		UE_LOG(LogKronos, Warning, TEXT("Matchmaking is already being canceled."));
		return;
	}

	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Canceling matchmaking..."));

	bWasCanceled = true;
	SetMatchmakingState(EKronosMatchmakingState::Canceling);

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_MatchmakingDelay);

	if (SearchPass && SearchPass->IsSearching())
	{
		MatchmakingAsyncStateFlags |= static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingSearch);
		if (!SearchPass->CancelSearch())
		{
			// Safety measure. In case the canceling doesn't start, the completion delegate wouldn't trigger either so the flow would get stuck.
			// We have to set the flag before canceling, because the cancel itself can finish in one frame.
			MatchmakingAsyncStateFlags &= ~static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingSearch);
		}
	}

	if (ReservationBeaconClient)
	{
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingReservationRequest))
		{
			// It looks like we are already canceling a reservation request. This means that a completion delegate will trigger somewhere in the matchmaking flow.
			// Most likely we are trying to clean up our previous reservation before continuing (e.g. requesting a new reservation).
			return;
		}

		// This will automatically finish canceling the matchmaking if it was the last async task.
		CleanupExistingReservations();
	}

	SignalCancelMatchmakingCompleteChecked();
}

void UKronosMatchmakingPolicy::Invalidate()
{
	if (SearchPass)
	{
		SearchPass->Invalidate();
		SearchPass = nullptr;
	}

	if (ReservationBeaconClient)
	{
		ReservationBeaconClient->DestroyBeacon();
		ReservationBeaconClient = nullptr;
	}

	OnKronosMatchmakingComplete().Clear();
	OnCancelKronosMatchmakingComplete().Clear();
	OnKronosMatchmakingStateChanged().Clear();

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		}
	}

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void UKronosMatchmakingPolicy::BeginMatchmaking()
{
	switch (MatchmakingMode)
	{
	case EKronosMatchmakingMode::Default:
		HandleStartingDefaultMatchmaking();
		break;
	case EKronosMatchmakingMode::CreateOnly:
		HandleStartingCreateOnlyMatchmaking();
		break;
	case EKronosMatchmakingMode::SearchOnly:
		HandleStartingSearchOnlyMatchmaking();
		break;
	case EKronosMatchmakingMode::JoinOnly:
		HandleStartingJoinOnlyMatchmaking();
		break;
	default:
		UE_LOG(LogKronos, Error, TEXT("Unhandled matchmaking mode when starting matchmaking."));
		break;
	}
}

void UKronosMatchmakingPolicy::HandleStartingDefaultMatchmaking()
{
	SetMatchmakingState(EKronosMatchmakingState::Searching);
	StartSearchPass();
}

void UKronosMatchmakingPolicy::HandleStartingCreateOnlyMatchmaking()
{
	CreateOnlineSession();
}

void UKronosMatchmakingPolicy::HandleStartingSearchOnlyMatchmaking()
{
	SetMatchmakingState(EKronosMatchmakingState::Searching);
	StartSearchPass();
}

void UKronosMatchmakingPolicy::HandleStartingJoinOnlyMatchmaking()
{
	if (SessionToJoin.IsValid())
	{
		int32 bSessionRequiresReservation = 0;
		SessionToJoin.GetSessionSetting(SETTING_USERESERVATIONS, bSessionRequiresReservation);

		if (bSessionRequiresReservation == 0 || MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::SkipReservation))
		{
			JoinOnlineSession(SessionToJoin);
			return;
		}

		RequestReservation(SessionToJoin);
		return;
	}

	UE_LOG(LogKronos, Error, TEXT("SessionToJoin is invalid!"));
	SignalMatchmakingComplete(EKronosMatchmakingState::Failure, EKronosMatchmakingCompleteResult::Failure, EKronosMatchmakingFailureReason::InvalidParams);
}

void UKronosMatchmakingPolicy::StartSearchPass()
{
	FKronosSearchParams SearchParams = FKronosSearchParams(MatchmakingParams, MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::SkipEloChecks));
	SearchParams.EloRange = GetEloSearchRangeFor(CurrentMatchmakingPassIdx);

	SearchPass->StartSearch(SessionName, SearchParams);
}

int32 UKronosMatchmakingPolicy::GetEloSearchRangeFor(int32 MatchmakingPassIdx)
{
	// Take the base EloRange and add EloSearchStep amount to it for each new matchmaking pass.
	// We only want to increase the base EloRange if we are not in the first pass (MatchmakingPassIdx - 1).
	return MatchmakingParams.EloRange + MatchmakingParams.EloSearchStep * (MatchmakingPassIdx - 1);
}

void UKronosMatchmakingPolicy::OnSearchPassComplete(const FName InSessionName, const EKronosSearchPassCompleteResult Result)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnSearchPassComplete with result: %s"), LexToString(Result));

	// Failure during search pass.
	if (Result == EKronosSearchPassCompleteResult::Failure)
	{
		SignalMatchmakingComplete(EKronosMatchmakingState::Failure, EKronosMatchmakingCompleteResult::Failure, EKronosMatchmakingFailureReason::SearchPassFailure);
		return;
	}

	// Search only matchmaking.
	if (MatchmakingMode == EKronosMatchmakingMode::SearchOnly)
	{
		switch (Result)
		{
		case EKronosSearchPassCompleteResult::NoSession:
			SignalMatchmakingComplete(EKronosMatchmakingState::Complete, EKronosMatchmakingCompleteResult::NoResults);
			return;
		case EKronosSearchPassCompleteResult::Success:
			SignalMatchmakingComplete(EKronosMatchmakingState::Complete, EKronosMatchmakingCompleteResult::Success);
			return;
		}
	}

	else
	{
		// If there is at least one session found, start testing.
		if (Result == EKronosSearchPassCompleteResult::Success)
		{
			StartTestingSearchResults();
			return;
		}

		// No sessions found, continue matchmaking.
		RestartMatchmaking();
		return;
	}
}

void UKronosMatchmakingPolicy::OnCancelSearchPassComplete()
{
	UE_LOG(LogKronos, Log, TEXT("Search pass canceled."));

	MatchmakingAsyncStateFlags &= ~static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingSearch);

	SignalCancelMatchmakingCompleteChecked();
}

void UKronosMatchmakingPolicy::StartTestingSearchResults()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Testing search results..."));

	CurrentSessionIdx = -1;
	ContinueTestingSearchResults();
}

void UKronosMatchmakingPolicy::ContinueTestingSearchResults()
{
	UE_LOG(LogKronos, Verbose, TEXT("Testing next search result..."));

	CurrentSessionIdx++;

	FKronosSearchResult SearchResult = FKronosSearchResult();
	if (SearchPass && SearchPass->GetSearchResult(CurrentSessionIdx, SearchResult))
	{
		int32 bSessionRequiresReservation = 0;
		SearchResult.GetSessionSetting(SETTING_USERESERVATIONS, bSessionRequiresReservation);

		if (bSessionRequiresReservation == 0 || MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::SkipReservation))
		{
			JoinOnlineSession(SearchResult);
			return;
		}

		RequestReservation(SearchResult);
		return;
	}

	else
	{
		RestartMatchmaking();
	}
}

void UKronosMatchmakingPolicy::RestartMatchmaking()
{
	// We don't want to go into a full matchmaking process. Just finish the matchmaking.
	if (MatchmakingMode != EKronosMatchmakingMode::Default)
	{
		SignalMatchmakingComplete(EKronosMatchmakingState::Complete, EKronosMatchmakingCompleteResult::NoResults);
		return;
	}

	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Matchmaking pass exhausted all options. Restarting..."));

	// Check if we have search attempts left.
	if (CurrentMatchmakingPassIdx < MatchmakingParams.MaxSearchAttempts)
	{
		// Switch over to hosting role if needed.
		if (!(MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::NoHost)))
		{
			int32 NextEloRange = GetEloSearchRangeFor(CurrentMatchmakingPassIdx + 1);
			if (NextEloRange >= MatchmakingParams.EloRangeBeforeHosting)
			{
				UE_LOG(LogKronos, Log, TEXT("Elo range limit reached. Switching over to hosting role..."));

				CurrentMatchmakingPassIdx++;

				CreateOnlineSession();
				return;
			}
		}

		UE_LOG(LogKronos, Log, TEXT("Widening Elo range and preparing another search..."));
		UE_LOG(LogKronos, Log, TEXT("Matchmaking attempt: %d/%d"), CurrentMatchmakingPassIdx + 1, MatchmakingParams.MaxSearchAttempts);

		SetMatchmakingState(EKronosMatchmakingState::Searching);

		FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this]()
		{
			CurrentMatchmakingPassIdx++;
			StartSearchPass();
		});

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_MatchmakingDelay, TimerDelegate, GetDefault<UKronosConfig>()->RestartMatchmakingPassDelay, false);
		return;
	}

	// Search attempt limit reached.
	else
	{
		UE_LOG(LogKronos, Log, TEXT("Search attempt limit reached."));

		// Switch over to hosting role if possible.
		if (!(MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::NoHost)))
		{
			CreateOnlineSession();
			return;
		}

		SignalMatchmakingComplete(EKronosMatchmakingState::Complete, EKronosMatchmakingCompleteResult::NoResults);
		return;
	}
}

void UKronosMatchmakingPolicy::SetMatchmakingState(const EKronosMatchmakingState InState)
{
	EKronosMatchmakingState OldState = MatchmakingState;
	if (InState != OldState)
	{
		KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Matchmaking state changed: %s -> %s"), LexToString(OldState), LexToString(InState));

		MatchmakingState = InState;
		OnKronosMatchmakingStateChanged().Broadcast(OldState, MatchmakingState);
		OnKronosMatchmakingUpdated().Broadcast(MatchmakingState, MatchmakingTime);
	}
}

void UKronosMatchmakingPolicy::SignalStartMatchmakingComplete()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Matchmaking started."));
	OnStartKronosMatchmakingComplete().Broadcast();

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	if (PartyManager->IsPartyLeader())
	{
		PartyManager->SetPartyLeaderMatchmaking(true);
	}
}

void UKronosMatchmakingPolicy::SignalMatchmakingComplete(const EKronosMatchmakingState EndState, const EKronosMatchmakingCompleteResult Result, const EKronosMatchmakingFailureReason Reason)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnMatchmakingComplete with result: %s"), LexToString(Result));
	KRONOS_CLOG(Result == EKronosMatchmakingCompleteResult::Failure, LogKronos, Log, COLOR_DARK_CYAN, TEXT("Failure reason: %s"), LexToString(Reason));
	bMatchmakingInProgress = false;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_MatchmakingTimer);

	MatchmakingResult = Result;
	if (Result == EKronosMatchmakingCompleteResult::Failure)
	{
		FailureReason = Reason;
	}

	SetMatchmakingState(EndState);
	OnKronosMatchmakingComplete().Broadcast(SessionName, Result);

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	if (PartyManager->IsPartyLeader())
	{
		PartyManager->SetPartyLeaderMatchmaking(false);
	}
}

void UKronosMatchmakingPolicy::SignalCancelMatchmakingComplete()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Matchmaking canceled."));
	bMatchmakingInProgress = false;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_MatchmakingTimer);

	SetMatchmakingState(EKronosMatchmakingState::Canceled);
	OnCancelKronosMatchmakingComplete().Broadcast();

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	if (PartyManager->IsPartyLeader())
	{
		PartyManager->SetPartyLeaderMatchmaking(false);
	}
}

bool UKronosMatchmakingPolicy::SignalCancelMatchmakingCompleteChecked()
{
	if (bWasCanceled && MatchmakingAsyncStateFlags == 0 && MatchmakingState != EKronosMatchmakingState::Canceled)
	{
		SignalCancelMatchmakingComplete();
		return true;
	}

	return false;
}

bool UKronosMatchmakingPolicy::CreateOnlineSession()
{
	SetMatchmakingState(EKronosMatchmakingState::CreatingSession);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// Make sure that we are not in a session.
			if (SessionInterface->GetSessionState(SessionName) != EOnlineSessionState::NoSession)
			{
				FOnDestroySessionCompleteDelegate CompletionDelegate = FOnDestroySessionCompleteDelegate::CreateLambda([this](FName SessionName, bool bWasSuccessful)
				{
					CreateOnlineSession();
				});

				CleanupExistingSession(SessionName, CompletionDelegate);
				return true;
			}

			// Make sure that we don't have an existing reservation.
			if (ReservationBeaconClient)
			{
				FOnCleanupKronosMatchmakingComplete CompletionDelegate = FOnCleanupKronosMatchmakingComplete::CreateLambda([this](bool bWasSuccessful)
				{
					CreateOnlineSession();
				});

				CleanupExistingReservations(CompletionDelegate);
				return true;
			}

			KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Creating %s..."), *SessionName.ToString());
			UE_CLOG(MatchmakingParams.HostParams.HasSessionSettingsOverride(), LogKronos, Log, TEXT("Override session settings detected. Session will be created using custom settings."));

			MatchmakingAsyncStateFlags |= static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CreatingSession);

			FOnlineSessionSettings SessionSettings = InitOnlineSessionSettings();

			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
			OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

			return SessionInterface->CreateSession(0, SessionName, SessionSettings);
		}
	}

	OnCreateSessionComplete(SessionName, false);
	return false;
}

FOnlineSessionSettings UKronosMatchmakingPolicy::InitOnlineSessionSettings()
{
	// Check if we have specific session settings given to us already.
	// This will be true in case we want to recreate a session from its previous settings.
	if (MatchmakingParams.HostParams.HasSessionSettingsOverride())
	{
		// Get the raw settings.
		FOnlineSessionSettings& SessionSettingsOverride = MatchmakingParams.HostParams.SessionSettingsOverride.GetValue();

		// Update the owner id because currently it is pointing to the previous session host's id.
		// The local player will be the new host since he is the one creating the session.
		FString OwnerUniqueIdString = UKronosStatics::GetLocalPlayerId(this)->ToString();
		SessionSettingsOverride.Set(SETTING_OWNERID, OwnerUniqueIdString, EOnlineDataAdvertisementType::ViaOnlineService);

		return SessionSettingsOverride;
	}

	// No override session settings are given.
	// We are creating a new session from the ground up.

	FOnlineSessionSettings SessionSettings = FOnlineSessionSettings();
	SessionSettings.NumPublicConnections = MatchmakingParams.HostParams.MaxNumPlayers;
	SessionSettings.bShouldAdvertise = MatchmakingParams.HostParams.bShouldAdvertise;
	SessionSettings.bAllowJoinInProgress = MatchmakingParams.HostParams.bAllowJoinInProgress;
	SessionSettings.bIsLANMatch = MatchmakingParams.HostParams.bIsLanMatch;
	SessionSettings.bUsesPresence = MatchmakingParams.HostParams.bUsesPresence;
	SessionSettings.bAllowInvites = MatchmakingParams.HostParams.bAllowInvites;
	SessionSettings.bAllowJoinViaPresence = MatchmakingParams.HostParams.bAllowJoinViaPresence;
	SessionSettings.bUseLobbiesVoiceChatIfAvailable = MatchmakingParams.HostParams.bUseVoiceChatIfAvailable;

	// This was added in UE_4.27 as preparation for OSSv2. It looks like this param will take over the purpose of the presence setting.
	// Presence session implies lobby API.
	SessionSettings.bUseLobbiesIfAvailable = MatchmakingParams.HostParams.bUsesPresence;

	// Session type is used to differentiate between game sessions and party sessions.
	SessionSettings.Set(SETTING_SESSIONTYPE, SessionName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);

	// Session owner id is used when we want to find a specific session (e.g. following party leader into a session).
	// Not using actual session ids because this can be set even before the session has been created.
	FString OwnerUniqueIdString = UKronosStatics::GetLocalPlayerId(this)->ToString();
	SessionSettings.Set(SETTING_OWNERID, OwnerUniqueIdString, EOnlineDataAdvertisementType::ViaOnlineService);

	// Beacon port is used by the online subsystem when resolving the connect string for beacons.
	int32 BeaconListenPort = GetPreferredBeaconPort();
	SessionSettings.Set(SETTING_BEACONPORT, BeaconListenPort, EOnlineDataAdvertisementType::ViaOnlineService);

	// Whether the session is hidden or not. Hidden sessions can only be found during a specific session query.
	// Converting to int32 because the Steam Subsystem doesn't support bool queries.
	int32 bHidden = MatchmakingParams.HostParams.bHidden ? 1 : 0;
	SessionSettings.Set(SETTING_HIDDEN, bHidden, EOnlineDataAdvertisementType::ViaOnlineService);

	// Whether to use the reservation system for the session. Defaults to true for game sessions.
	// Converting to int32 because the Steam Subsystem doesn't support bool queries.
	int32 bUseReservations = SessionName == NAME_GameSession ? 1 : 0;
	SessionSettings.Set(SETTING_USERESERVATIONS, bUseReservations, EOnlineDataAdvertisementType::ViaOnlineService);

	// Session display name (server name). Defaults to hosting player's unique id.
	bool bHasServerName = !MatchmakingParams.HostParams.ServerName.IsEmpty();
	SessionSettings.Set(SETTING_SERVERNAME, bHasServerName ? MatchmakingParams.HostParams.ServerName : GetDefaultServerName(), EOnlineDataAdvertisementType::ViaOnlineService);

	// Playlist name.
	if (!MatchmakingParams.HostParams.Playlist.IsEmpty())
	{
		SessionSettings.Set(SETTING_PLAYLIST, MatchmakingParams.HostParams.Playlist, EOnlineDataAdvertisementType::ViaOnlineService);
	}

	// Map name.
	if (!MatchmakingParams.HostParams.MapName.IsEmpty())
	{
		SessionSettings.Set(SETTING_MAPNAME, MatchmakingParams.HostParams.MapName, EOnlineDataAdvertisementType::ViaOnlineService);
	}

	// Game mode name.
	if (!MatchmakingParams.HostParams.GameMode.IsEmpty())
	{
		SessionSettings.Set(SETTING_GAMEMODE, MatchmakingParams.HostParams.GameMode, EOnlineDataAdvertisementType::ViaOnlineService);
	}

	// Session Elo rating. Describes the average skill level of players in the session.
	// We have to use two different session settings, because query settings can only compare against one session setting at a time.
	SessionSettings.Set(SETTING_SESSIONELO, MatchmakingParams.HostParams.Elo, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_SESSIONELO2, MatchmakingParams.HostParams.Elo, EOnlineDataAdvertisementType::ViaOnlineService);

	// Starting level is used to select which map should be opened by the host when calling UKronosMatchmakingOnlineSession::ServerTravelToSession().
	// Not advertised because this is only relevant to the session's host. Only ever used by game sessions.
	if (SessionName == NAME_GameSession)
	{
		SessionSettings.Set(SETTING_STARTINGLEVEL, MatchmakingParams.HostParams.StartingLevel, EOnlineDataAdvertisementType::DontAdvertise);
	}

	// Players who are not allowed to join the session.
	if (MatchmakingParams.HostParams.BannedPlayers.Num() > 0)
	{
		// The expected format is "uniqueid1;uniqueid2;uniqueid3".

		FString BannedPlayersString;
		for (int32 Idx = 0; Idx < MatchmakingParams.HostParams.BannedPlayers.Num(); Idx++)
		{
			const FUniqueNetIdRepl& BannedPlayerUniqueId = MatchmakingParams.HostParams.BannedPlayers[Idx];
			if (BannedPlayerUniqueId.IsValid())
			{
				bool bAppendComma = Idx > 0; // Append comma before every unique id except the first
				BannedPlayersString.Appendf(TEXT("%s%s"), bAppendComma ? TEXT(";") : TEXT(""), *BannedPlayerUniqueId.ToString());
			}
		}

		SessionSettings.Set(SETTING_BANNEDPLAYERS, BannedPlayersString, EOnlineDataAdvertisementType::ViaOnlineService);
	}

	// Extra session settings.
	for (const FKronosSessionSetting& ExtraSetting : MatchmakingParams.HostParams.ExtraSessionSettings)
	{
		if (ExtraSetting.IsValid())
		{
			// Register the session setting the same way as SessionSettings.Set(...) would
			{
				FOnlineSessionSetting* Setting = SessionSettings.Settings.Find(ExtraSetting.Key);
				if (Setting)
				{
					Setting->Data = ExtraSetting.Data;
					Setting->AdvertisementType = ExtraSetting.AdvertisementType;
				}
				else
				{
					SessionSettings.Settings.Add(ExtraSetting.Key, FOnlineSessionSetting(ExtraSetting.Data, ExtraSetting.AdvertisementType));
				}
			}
		}
	}

	return SessionSettings;
}

int32 UKronosMatchmakingPolicy::GetPreferredBeaconPort()
{
	// Default beacon port.
	int32 BeaconListenPort = DEFAULT_BEACON_PORT;

	// Attempt to get the beacon port from the base OnlineBeaconHost class.
	GConfig->GetInt(TEXT("/Script/OnlineSubsystemUtils.OnlineBeaconHost"), TEXT("ListenPort"), BeaconListenPort, GEngineIni);

	// Attempt to get the session specific beacon port.
	// Game sessions use reservation beacons while party sessions use party beacons.
	const TCHAR* ConfigPath = SessionName == NAME_GameSession ? TEXT("/Script/Kronos.KronosReservationListener") : TEXT("/Script/Kronos.KronosPartyListener");
	GConfig->GetInt(ConfigPath, TEXT("ListenPort"), BeaconListenPort, GEngineIni);

	// Allow the command line to override the beacon port.
	int32 PortOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("BeaconPort="), PortOverride))
	{
		BeaconListenPort = PortOverride;
	}

	return BeaconListenPort;
}

FString UKronosMatchmakingPolicy::GetDefaultServerName()
{
	FString PlayerNickname = UKronosStatics::GetPlayerNickname(this);
	if (!PlayerNickname.IsEmpty())
	{
		return FString::Printf(TEXT("%s's Session"), *PlayerNickname);
	}
	
	return UKronosStatics::GetLocalPlayerId(this).ToString().Left(20);
}

void UKronosMatchmakingPolicy::OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnCreateSessionComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	MatchmakingAsyncStateFlags &= ~static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CreatingSession);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);

			if (bWasCanceled)
			{
				FOnDestroySessionCompleteDelegate CompletionDelegate = FOnDestroySessionCompleteDelegate::CreateLambda([this](FName SessionName, bool bWasSuccessful)
				{
					SignalCancelMatchmakingCompleteChecked();
				});

				CleanupExistingSession(SessionName, CompletionDelegate);
				return;
			}

			if (bWasSuccessful)
			{
				SignalMatchmakingComplete(EKronosMatchmakingState::Complete, EKronosMatchmakingCompleteResult::SessionCreated);
				return;
			}
		}
	}

	SignalMatchmakingComplete(EKronosMatchmakingState::Failure, EKronosMatchmakingCompleteResult::Failure, EKronosMatchmakingFailureReason::CreateSessionFailure);
}

bool UKronosMatchmakingPolicy::RequestReservation(const FKronosSearchResult& InSession)
{
	SetMatchmakingState(EKronosMatchmakingState::RequestingReservation);

	// Make sure that we don't have an existing reservation.
	if (ReservationBeaconClient)
	{
		FOnCleanupKronosMatchmakingComplete CompletionDelegate = FOnCleanupKronosMatchmakingComplete::CreateLambda([this, InSession](bool bWasSuccessful)
		{
			RequestReservation(InSession);
		});

		CleanupExistingReservations(CompletionDelegate);
		return true;
	}

	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Requesting reservation..."));

	ReservationBeaconClient = GetWorld()->SpawnActor<AKronosReservationClient>(GetReservationClientClass());
	if (ReservationBeaconClient)
	{
		MatchmakingAsyncStateFlags |= static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::RequestingReservation);

		FKronosReservation PartyReservation = UKronosStatics::MakeReservationForParty(this);
		FOnKronosReservationRequestComplete CompletionDelegate = FOnKronosReservationRequestComplete::CreateUObject(this, &ThisClass::OnRequestReservationComplete);

		return ReservationBeaconClient->RequestReservation(InSession, PartyReservation, CompletionDelegate);
	}

	OnRequestReservationComplete(InSession, EKronosReservationCompleteResult::ConnectionError);
	return false;
}

void UKronosMatchmakingPolicy::OnRequestReservationComplete(const FKronosSearchResult& SearchResult, const EKronosReservationCompleteResult Result)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnRequestReservationComplete with result: %s"), LexToString(Result));

	MatchmakingAsyncStateFlags &= ~static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::RequestingReservation);

	if (Result == EKronosReservationCompleteResult::ReservationAccepted)
	{
		if (SearchResult.IsValid())
		{
			JoinOnlineSession(SearchResult);
			return;
		}
	}

	else
	{
		ContinueTestingSearchResults();
		return;
	}
}

bool UKronosMatchmakingPolicy::JoinOnlineSession(const FKronosSearchResult& InSession)
{
	SetMatchmakingState(EKronosMatchmakingState::JoiningSession);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid() && InSession.IsValid())
		{
			// Make sure that we are not in a session.
			if (SessionInterface->GetSessionState(SessionName) != EOnlineSessionState::NoSession)
			{
				FOnDestroySessionCompleteDelegate CompletionDelegate = FOnDestroySessionCompleteDelegate::CreateLambda([this, InSession](FName SessionName, bool bWasSuccessful)
				{
					JoinOnlineSession(InSession);
				});

				CleanupExistingSession(SessionName, CompletionDelegate);
				return true;
			}

			KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Joining %s..."), *SessionName.ToString());

			// Make sure we are not attempting to join a session which we or any of our party members are banned from.
			{
				UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
				if (PartyManager->IsPartyLeader())
				{
					if (InSession.IsPlayerBannedFromSession(PartyManager->GetPartyPlayerUniqueIds()))
					{
						OnJoinSessionComplete(SessionName, EOnJoinSessionCompleteResult::UnknownError);
						return true;
					}
				}

				else
				{
					FUniqueNetIdRepl LocalPlayerId = UKronosStatics::GetLocalPlayerId(this);
					if (InSession.IsPlayerBannedFromSession(LocalPlayerId))
					{
						OnJoinSessionComplete(SessionName, EOnJoinSessionCompleteResult::UnknownError);
						return true;
					}
				}
			}

			MatchmakingAsyncStateFlags |= static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::JoiningSession);

			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
			OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

			return SessionInterface->JoinSession(0, SessionName, InSession.OnlineResult);
		}
	}

	OnJoinSessionComplete(SessionName, EOnJoinSessionCompleteResult::UnknownError);
	return false;
}

void UKronosMatchmakingPolicy::OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnJoinSessionComplete with result: %s"), LexToString(Result));

	MatchmakingAsyncStateFlags &= ~static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::JoiningSession);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

			if (bWasCanceled)
			{
				FOnDestroySessionCompleteDelegate CompletionDelegate = FOnDestroySessionCompleteDelegate::CreateLambda([this](FName SessionName, bool bWasSuccessful)
				{
					SignalCancelMatchmakingCompleteChecked();
				});

				CleanupExistingSession(SessionName, CompletionDelegate);
				return;
			}

			if (Result == EOnJoinSessionCompleteResult::Success)
			{
				SignalMatchmakingComplete(EKronosMatchmakingState::Complete, EKronosMatchmakingCompleteResult::SessionJoined);
				return;
			}

			else
			{
				FOnCleanupKronosMatchmakingComplete CompletionDelegate = FOnCleanupKronosMatchmakingComplete::CreateLambda([this](bool bWasSuccessful)
				{
					ContinueTestingSearchResults();
				});

				CleanupExistingReservations(CompletionDelegate);
				return;
			}
		}
	}

	SignalMatchmakingComplete(EKronosMatchmakingState::Failure, EKronosMatchmakingCompleteResult::Failure, EKronosMatchmakingFailureReason::JoinSessionFailure);
}

bool UKronosMatchmakingPolicy::CleanupExistingSession(const FName InSessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Cleaning up existing %s..."), *InSessionName.ToString());

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
			if (OnlineSession)
			{
				// The delegate that will trigger once the session is destroyed.
				FOnDestroySessionCompleteDelegate DestroySessionCompletionDelegate = FOnDestroySessionCompleteDelegate::CreateLambda([this, CompletionDelegate](FName SessionName, bool bWasSuccessful)
				{
					// The delegate that will trigger after the one-frame delay.
					FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this, CompletionDelegate, SessionName, bWasSuccessful]()
					{
						KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnCleanupExistingSession complete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));
						CompletionDelegate.ExecuteIfBound(SessionName, bWasSuccessful);
					});

					// Additional one-frame delay for safety measures.
					GetWorld()->GetTimerManager().SetTimerForNextTick(TimerDelegate);
				});

				OnlineSession->DestroySession(InSessionName, DestroySessionCompletionDelegate);
				return true;
			}
		}
	}

	CompletionDelegate.ExecuteIfBound(SessionName, false);
	return false;
}

bool UKronosMatchmakingPolicy::CleanupExistingReservations(const FOnCleanupKronosMatchmakingComplete& CompletionDelegate)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Cleaning up existing reservations..."));

	if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingReservationRequest))
	{
		// If we ever hit this something went wrong, or we encountered some unforeseen edge case. It looks like we are already canceling a reservation request.
		// This means that a completion delegate for it must trigger somewhere in the matchmaking flow. We'll let that delegate take control over what happens next.

		UE_LOG(LogKronos, Error, TEXT("Failed to cancel previous reservation when requesting a new one, because we are already canceling a reservation request."));
		return false;
	}

	if (ReservationBeaconClient)
	{
		// Delegate that will trigger once the reservation has been canceled.
		FOnCancelKronosReservationComplete CancelReservationCompletionDelegate = FOnCancelKronosReservationComplete::CreateLambda([this, CompletionDelegate](bool bWasSuccessful)
		{
			// The delegate that will trigger after the one-frame delay. At this point, it will be safe to destroy the beacon.
			FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this, CompletionDelegate, bWasSuccessful]()
			{
				KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnCleanupExistingReservations complete with result: %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failure"));
				MatchmakingAsyncStateFlags &= ~static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingReservationRequest);

				if (ReservationBeaconClient)
				{
					ReservationBeaconClient->DestroyBeacon();
					ReservationBeaconClient = nullptr;
				}

				// In theory, it is possible that we started canceling the matchmaking while canceling the previous reservation.
				if (bWasCanceled)
				{
					SignalCancelMatchmakingCompleteChecked();
					return;
				}

				CompletionDelegate.ExecuteIfBound(true);
			});

			// Additional one-frame delay, because we can't destroy the beacon while it is executing its delegate.
			GetWorld()->GetTimerManager().SetTimerForNextTick(TimerDelegate);
		});

		MatchmakingAsyncStateFlags |= static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingReservationRequest);
		if (!ReservationBeaconClient->CancelReservation(CancelReservationCompletionDelegate))
		{
			// Safety measure. In case the canceling doesn't start, the completion delegate wouldn't trigger either so the flow would get stuck.
			// We have to set the flag before canceling, because the cancel itself can finish in one frame.

			MatchmakingAsyncStateFlags &= ~static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingReservationRequest);
			return false;
		}

		// Remove the 'requesting reservation' flag because the response to that will never come.
		MatchmakingAsyncStateFlags &= ~static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::RequestingReservation);

		// At this point the cancel reservation completion delegate will trigger.
		return true;
	}

	CompletionDelegate.ExecuteIfBound(true);
	return true;
}

TSubclassOf<UKronosMatchmakingSearchPass> UKronosMatchmakingPolicy::GetSearchPassClass()
{
	return GetDefault<UKronosConfig>()->MatchmakingSearchPassClass;
}

TSubclassOf<AKronosReservationClient> UKronosMatchmakingPolicy::GetReservationClientClass()
{
	return GetDefault<UKronosConfig>()->ReservationClientClass;
}

void UKronosMatchmakingPolicy::DumpSettings() const
{
	UE_LOG(LogKronos, Log, TEXT("Dumping matchmaking settings..."));
	UE_LOG(LogKronos, Log, TEXT("  SessionName: %s"), *SessionName.ToString());
	UE_LOG(LogKronos, Log, TEXT("  Params:"));
	UE_LOG(LogKronos, Log, TEXT("    HostParams:"));
	UE_LOG(LogKronos, Log, TEXT("      Playlist: %s"), *MatchmakingParams.HostParams.Playlist);
	UE_LOG(LogKronos, Log, TEXT("      MapName: %s"), *MatchmakingParams.HostParams.MapName);
	UE_LOG(LogKronos, Log, TEXT("      GameMode: %s"), *MatchmakingParams.HostParams.GameMode);
	UE_LOG(LogKronos, Log, TEXT("      MaxNumPlayers: %d"), MatchmakingParams.HostParams.MaxNumPlayers);
	UE_LOG(LogKronos, Log, TEXT("      Elo: %d"), MatchmakingParams.HostParams.Elo);
	UE_LOG(LogKronos, Log, TEXT("      bShouldAdvertise: %s"), MatchmakingParams.HostParams.bShouldAdvertise ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("      bHidden: %s"), MatchmakingParams.HostParams.bHidden ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("      bAllowJoinInProgress: %s"), MatchmakingParams.HostParams.bAllowJoinInProgress ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("      bIsLanMatch: %s"), MatchmakingParams.HostParams.bIsLanMatch ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("      bUsesPresence: %s"), MatchmakingParams.HostParams.bUsesPresence ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("      bAllowInvites: %s"), MatchmakingParams.HostParams.bAllowInvites ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("      bAllowJoinViaPresence: %s"), MatchmakingParams.HostParams.bAllowJoinViaPresence ? TEXT("True") : TEXT("False"));

	UE_LOG(LogKronos, Log, TEXT("      ExtraSessionSettings: %s"), MatchmakingParams.HostParams.ExtraSessionSettings.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FKronosSessionSetting& ExtraSetting : MatchmakingParams.HostParams.ExtraSessionSettings)
	{
		UE_LOG(LogKronos, Log, TEXT("        %s=%s (%s)"), *ExtraSetting.Key.ToString(), *ExtraSetting.Data.ToString(), EOnlineDataAdvertisementType::ToString(ExtraSetting.AdvertisementType));
	}

	UE_LOG(LogKronos, Log, TEXT("      BannedPlayers: %s"), MatchmakingParams.HostParams.BannedPlayers.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FUniqueNetIdRepl& BannedPlayer : MatchmakingParams.HostParams.BannedPlayers)
	{
		UE_LOG(LogKronos, Log, TEXT("        %s"), *BannedPlayer.ToDebugString());
	}

	UE_LOG(LogKronos, Log, TEXT("    Playlist: %s"), *MatchmakingParams.Playlist);
	UE_LOG(LogKronos, Log, TEXT("    MapName: %s"), *MatchmakingParams.MapName);
	UE_LOG(LogKronos, Log, TEXT("    GameMode: %s"), *MatchmakingParams.GameMode);
	UE_LOG(LogKronos, Log, TEXT("    MaxSearchAttempts: %d"), MatchmakingParams.MaxSearchAttempts);
	UE_LOG(LogKronos, Log, TEXT("    MaxSearchResults: %d"), MatchmakingParams.MaxSearchResults);
	UE_LOG(LogKronos, Log, TEXT("    MinSlotsRequired: %d"), MatchmakingParams.MinSlotsRequired);
	UE_LOG(LogKronos, Log, TEXT("    Elo: %d"), MatchmakingParams.Elo);
	UE_LOG(LogKronos, Log, TEXT("    EloRange: %d"), MatchmakingParams.EloRange);
	UE_LOG(LogKronos, Log, TEXT("    EloSearchAttempts: %d"), MatchmakingParams.EloSearchAttempts);
	UE_LOG(LogKronos, Log, TEXT("    EloSearchStep: %d"), MatchmakingParams.EloSearchStep);
	UE_LOG(LogKronos, Log, TEXT("    EloRangeBeforeHosting: %s"), MatchmakingParams.EloRangeBeforeHosting != MAX_int32 ? *LexToString(MatchmakingParams.EloRangeBeforeHosting) : TEXT("MAX_int32"));
	UE_LOG(LogKronos, Log, TEXT("    bIsLanQuery: %s"), MatchmakingParams.bIsLanQuery ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("    bSearchPresence: %s"), MatchmakingParams.bSearchPresence ? TEXT("True") : TEXT("False"));

	FString SpecificSessionQueryString = FString::Printf(TEXT("[%s] %s"), LexToString(MatchmakingParams.SpecificSessionQuery.Type), *MatchmakingParams.SpecificSessionQuery.UniqueId.ToDebugString());
	UE_LOG(LogKronos, Log, TEXT("    SpecificSessionQuery: %s"), MatchmakingParams.IsSpecificSessionQuery() ? *SpecificSessionQueryString : TEXT("-"));

	UE_LOG(LogKronos, Log, TEXT("    ExtraQuerySettings: %s"), MatchmakingParams.ExtraQuerySettings.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FKronosQuerySetting& ExtraSetting : MatchmakingParams.ExtraQuerySettings)
	{
		UE_LOG(LogKronos, Log, TEXT("      %s=%s (%s)"), *ExtraSetting.Key.ToString(), *ExtraSetting.Data.ToString(), EOnlineComparisonOp::ToString(ExtraSetting.ComparisonOp));
	}

	UE_LOG(LogKronos, Log, TEXT("    IgnoredSessions: %s"), MatchmakingParams.IgnoredSessions.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FUniqueNetIdRepl& IgnoredSession : MatchmakingParams.IgnoredSessions)
	{
		UE_LOG(LogKronos, Log, TEXT("      %s"), *IgnoredSession.ToDebugString());
	}

	FString ActiveFlags = TEXT("None");
	if (MatchmakingFlags != 0)
	{
		ActiveFlags.Empty();
		if (MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::NoHost)) ActiveFlags.Append(TEXT("-NoHost "));
		if (MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::SkipReservation)) ActiveFlags.Append(TEXT("-SkipReservation "));
		if (MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::SkipEloChecks)) ActiveFlags.Append(TEXT("-SkipEloChecks "));
	}

	UE_LOG(LogKronos, Log, TEXT("  Flags: %s"), *ActiveFlags);
	UE_LOG(LogKronos, Log, TEXT("  Mode: %s"), LexToString(MatchmakingMode));
}

void UKronosMatchmakingPolicy::DumpMatchmakingState() const
{
	FString ActiveFlags = TEXT("None");
	if (MatchmakingAsyncStateFlags != 0)
	{
		ActiveFlags.Empty();
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingSearch)) ActiveFlags.Append(TEXT("-CancelingSearch "));
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CreatingSession)) ActiveFlags.Append(TEXT("-CreatingSession "));
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::JoiningSession)) ActiveFlags.Append(TEXT("-JoiningSession "));
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::RequestingReservation)) ActiveFlags.Append(TEXT("-RequestingReservation "));
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingReservationRequest)) ActiveFlags.Append(TEXT("-CancelingReservationRequest "));
	}

	UE_LOG(LogKronos, Log, TEXT("Dumping matchmaking state..."));
	UE_LOG(LogKronos, Log, TEXT("  bMatchmakingInProgress: %s"), bMatchmakingInProgress ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("  bWasCanceled: %s"), bWasCanceled ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("  MatchmakingState: %s"), LexToString(MatchmakingState));
	UE_LOG(LogKronos, Log, TEXT("  MatchmakingTime: %d"), MatchmakingTime);
	UE_LOG(LogKronos, Log, TEXT("  CurrentMatchmakingPassIdx: %d"), CurrentMatchmakingPassIdx);
	UE_LOG(LogKronos, Log, TEXT("  MatchmakingAsyncStateFlags: %s"), *ActiveFlags);
}

FString UKronosMatchmakingPolicy::GetDebugString() const
{
	FString DebugString;

	FString ParamFlags = TEXT("None");
	if (MatchmakingFlags != 0)
	{
		ParamFlags.Empty();
		if (MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::NoHost)) ParamFlags.Append(TEXT("-NoHost "));
		if (MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::SkipReservation)) ParamFlags.Append(TEXT("-SkipReservation "));
		if (MatchmakingFlags & static_cast<uint8>(EKronosMatchmakingFlags::SkipEloChecks)) ParamFlags.Append(TEXT("-SkipEloChecks "));
	}

	FString AsyncFlags = TEXT("None");
	if (MatchmakingAsyncStateFlags != 0)
	{
		AsyncFlags.Empty();
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingSearch)) AsyncFlags.Append(TEXT("-CancelingSearch "));
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CreatingSession)) AsyncFlags.Append(TEXT("-CreatingSession "));
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::JoiningSession)) AsyncFlags.Append(TEXT("-JoiningSession "));
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::RequestingReservation)) AsyncFlags.Append(TEXT("-RequestingReservation "));
		if (MatchmakingAsyncStateFlags & static_cast<uint8>(EKronosMatchmakingAsyncStateFlags::CancelingReservationRequest)) AsyncFlags.Append(TEXT("-CancelingReservationRequest "));
	}

	DebugString.Appendf(TEXT("{grey}Matchmaking:\n"));
	DebugString.Appendf(TEXT("\tMatchmakingState: {yellow}%s\n"), LexToString(MatchmakingState));
	DebugString.Appendf(TEXT("\tMatchmakingMode: {yellow}%s\n"), LexToString(MatchmakingMode));
	DebugString.Appendf(TEXT("\tSessionName: {yellow}%s\n"), *SessionName.ToString());
	DebugString.Appendf(TEXT("\tMatchmakingFlags: {yellow}%s\n"), *ParamFlags);
	DebugString.Appendf(TEXT("\tMatchmakingTime: {yellow}%d\n"), MatchmakingTime);
	DebugString.Appendf(TEXT("\tMatchmakingAsyncStateFlags: {yellow}%s\n"), *AsyncFlags);
	DebugString.Appendf(TEXT("\tMatchmakingParams:\n"));
	DebugString.Appendf(TEXT("\t\tHostParams:\n"));
	DebugString.Appendf(TEXT("\t\t\tPlaylist: {yellow}%s\n"), !MatchmakingParams.HostParams.Playlist.IsEmpty() ? *MatchmakingParams.HostParams.Playlist : TEXT("-"));
	DebugString.Appendf(TEXT("\t\t\tMapName: {yellow}%s\n"), !MatchmakingParams.HostParams.MapName.IsEmpty() ? *MatchmakingParams.HostParams.MapName : TEXT("-"));
	DebugString.Appendf(TEXT("\t\t\tGameMode: {yellow}%s\n"), !MatchmakingParams.HostParams.GameMode.IsEmpty() ? *MatchmakingParams.HostParams.GameMode : TEXT("-"));
	DebugString.Appendf(TEXT("\t\t\tMaxNumPlayers: {yellow}%d\n"), MatchmakingParams.HostParams.MaxNumPlayers);
	DebugString.Appendf(TEXT("\t\t\tElo: {yellow}%d\n"), MatchmakingParams.HostParams.Elo);
	DebugString.Appendf(TEXT("\t\t\tbShouldAdvertise: {yellow}%s\n"), MatchmakingParams.HostParams.bShouldAdvertise ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\t\t\tbHidden: {yellow}%s\n"), MatchmakingParams.HostParams.bHidden ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\t\t\tbAllowJoinInProgress: {yellow}%s\n"), MatchmakingParams.HostParams.bAllowJoinInProgress ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\t\t\tbIsLanMatch: {yellow}%s\n"), MatchmakingParams.HostParams.bIsLanMatch ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\t\t\tbUsesPresence: {yellow}%s\n"), MatchmakingParams.HostParams.bUsesPresence ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\t\t\tbAllowInvites: {yellow}%s\n"), MatchmakingParams.HostParams.bAllowInvites ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\t\t\tbAllowJoinViaPresence: {yellow}%s\n"), MatchmakingParams.HostParams.bAllowJoinViaPresence ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\t\t\tExtraSessionSettings: {yellow}%s\n"), MatchmakingParams.HostParams.ExtraSessionSettings.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FKronosSessionSetting& ExtraSetting : MatchmakingParams.HostParams.ExtraSessionSettings)
	{
		DebugString.Appendf(TEXT("\t\t\t\t%s = {yellow}%s (%s)\n"), *ExtraSetting.Key.ToString(), *ExtraSetting.Data.ToString(), EOnlineDataAdvertisementType::ToString(ExtraSetting.AdvertisementType));
	}
	DebugString.Appendf(TEXT("\t\t\tBannedPlayers: {yellow}%s\n"), MatchmakingParams.HostParams.BannedPlayers.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FUniqueNetIdRepl& BannedPlayer : MatchmakingParams.HostParams.BannedPlayers)
	{
		DebugString.Appendf(TEXT("\t\t\t\t%s\n"), *BannedPlayer.ToDebugString());
	}
	DebugString.Appendf(TEXT("\t\tPlaylist: {yellow}%s\n"), !MatchmakingParams.Playlist.IsEmpty() ? *MatchmakingParams.Playlist : TEXT("-"));
	DebugString.Appendf(TEXT("\t\tMapName: {yellow}%s\n"), !MatchmakingParams.MapName.IsEmpty() ? *MatchmakingParams.MapName : TEXT("-"));
	DebugString.Appendf(TEXT("\t\tGameMode: {yellow}%s\n"), !MatchmakingParams.GameMode.IsEmpty() ? *MatchmakingParams.GameMode : TEXT("-"));
	DebugString.Appendf(TEXT("\t\tMaxSearchAttempts: {yellow}%d\n"), MatchmakingParams.MaxSearchAttempts);
	DebugString.Appendf(TEXT("\t\tMaxSearchResults: {yellow}%d\n"), MatchmakingParams.MaxSearchResults);
	DebugString.Appendf(TEXT("\t\tMinSlotsRequired: {yellow}%d\n"), MatchmakingParams.MinSlotsRequired);
	DebugString.Appendf(TEXT("\t\tElo: {yellow}%d\n"), MatchmakingParams.Elo);
	DebugString.Appendf(TEXT("\t\tEloRange: {yellow}%d\n"), MatchmakingParams.EloRange);
	DebugString.Appendf(TEXT("\t\tEloSearchAttempts: {yellow}%d\n"), MatchmakingParams.EloSearchAttempts);
	DebugString.Appendf(TEXT("\t\tEloSearchStep: {yellow}%d\n"), MatchmakingParams.EloSearchStep);
	DebugString.Appendf(TEXT("\t\tEloRangeBeforeHosting: {yellow}%s\n"), MatchmakingParams.EloRangeBeforeHosting != MAX_int32 ? *LexToString(MatchmakingParams.EloRangeBeforeHosting) : TEXT("MAX_int32"));
	DebugString.Appendf(TEXT("\t\tbIsLanQuery: {yellow}%s\n"), MatchmakingParams.bIsLanQuery ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\t\tbSearchPresence: {yellow}%s\n"), MatchmakingParams.bSearchPresence ? TEXT("True") : TEXT("False"));
	FString SpecificSessionQueryString = FString::Printf(TEXT("[%s] %s"), LexToString(MatchmakingParams.SpecificSessionQuery.Type), *MatchmakingParams.SpecificSessionQuery.UniqueId.ToDebugString());
	DebugString.Appendf(TEXT("\t\tSpecificSessionQuery: {yellow}%s\n"), MatchmakingParams.IsSpecificSessionQuery() ? *SpecificSessionQueryString : TEXT("-"));

	DebugString.Appendf(TEXT("\t\tExtraQuerySettings: {yellow}%s\n"), MatchmakingParams.ExtraQuerySettings.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FKronosQuerySetting& ExtraSetting : MatchmakingParams.ExtraQuerySettings)
	{
		DebugString.Appendf(TEXT("\t\t\t%s = {yellow}%s (%s)\n"), *ExtraSetting.Key.ToString(), *ExtraSetting.Data.ToString(), EOnlineComparisonOp::ToString(ExtraSetting.ComparisonOp));
	}

	DebugString.Appendf(TEXT("\t\tIgnoredSessions: {yellow}%s\n"), MatchmakingParams.IgnoredSessions.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FUniqueNetIdRepl& IgnoredSession : MatchmakingParams.IgnoredSessions)
	{
		DebugString.Appendf(TEXT("\t\t\t%s\n"), *IgnoredSession.ToDebugString());
	}

	return DebugString;
}
