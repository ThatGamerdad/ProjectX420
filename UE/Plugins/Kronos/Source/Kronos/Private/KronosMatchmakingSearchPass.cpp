// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosMatchmakingSearchPass.h"
#include "KronosPartyManager.h"
#include "Kronos.h"
#include "KronosConfig.h"
#include "OnlineSubsystem.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UKronosMatchmakingSearchPass::UKronosMatchmakingSearchPass(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindOnlineSessionsComplete);
		OnFindFriendSessionCompleteDelegate = FOnFindFriendSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnFindFriendSessionComplete);
		OnFindSessionByIdCompleteDelegate = FOnSingleSessionResultCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionByIdComplete);
		OnCancelFindSessionsCompleteDelegate = FOnCancelFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnCancelFindSessionsComplete);
	}
}

bool UKronosMatchmakingSearchPass::StartSearch(const FName InSessionName, const FKronosSearchParams& InParams)
{
	if (InSessionName != NAME_GameSession && InSessionName != NAME_PartySession)
	{
		UE_LOG(LogKronos, Warning, TEXT("SessionName is invalid. Make sure to use either 'NAME_GameSession' or 'NAME_PartySession'."));
		UE_LOG(LogKronos, Error, TEXT("Failed to start search pass!"));

		SignalSearchPassComplete(EKronosSearchPassState::NotStarted, EKronosSearchPassCompleteResult::Failure);
		return false;
	}

	if (!InParams.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("Failed to start search pass!"));

		SignalSearchPassComplete(EKronosSearchPassState::NotStarted, EKronosSearchPassCompleteResult::Failure);
		return false;
	}

	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosMatchmakingSearchPass: Starting search pass..."));

	SessionName = InSessionName;
	SearchParams = InParams;

	if (UE_LOG_ACTIVE(LogKronos, Verbose))
	{
		DumpSettings();
	}

	bWasCanceled = false;
	CurrentAttemptIdx = 0;

	BeginSearchAttempt();
	return true;
}

bool UKronosMatchmakingSearchPass::CancelSearch()
{
	if (!IsSearching())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosMatchmakingSearchPass: There is no search to cancel."));
		return false;
	}

	if (SearchState == EKronosSearchPassState::Canceling)
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosMatchmakingSearchPass: Search pass is already being canceled."));
		return false;
	}

	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosMatchmakingSearchPass: Canceling search..."));

	// Possible states when canceling:
	//  - Finding sessions
	//  - Pinging sessions
	//  - Timer running that will restart the search pass

	bWasCanceled = true;
	SearchState = EKronosSearchPassState::Canceling;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_SearchDelay);

	if (AsyncStateFlags & static_cast<uint8>(EKronosSearchPassAsyncStateFlags::FindingSessions))
	{
		CancelFindSessions();
		return true;
	}

	SignalCancelSearchPassCompleteChecked();
	return true;
}

bool UKronosMatchmakingSearchPass::IsSearching() const
{
	return SearchState == EKronosSearchPassState::Searching || SearchState == EKronosSearchPassState::PingingSessions;
}

bool UKronosMatchmakingSearchPass::GetSearchResult(const int32 InSessionIdx, FKronosSearchResult& OutSearchResult)
{
	if (FilteredSessions.IsValidIndex(InSessionIdx))
	{
		OutSearchResult = FilteredSessions[InSessionIdx];
		return true;
	}

	return false;
}

void UKronosMatchmakingSearchPass::Invalidate()
{
	OnSearchPassComplete().Unbind();
	OnCancelSearchPassComplete().Unbind();

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
			SessionInterface->ClearOnFindFriendSessionCompleteDelegate_Handle(0, OnFindFriendSessionCompleteDelegateHandle);
			SessionInterface->ClearOnCancelFindSessionsCompleteDelegate_Handle(OnCancelFindSessionsCompleteDelegateHandle);
		}
	}

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void UKronosMatchmakingSearchPass::BeginSearchAttempt()
{
	CurrentAttemptIdx++;

	switch (SearchParams.SpecificSessionQuery.Type)
	{
	case EKronosSpecificSessionQueryType::FriendId:
		FindFriendSession();
		break;
	case EKronosSpecificSessionQueryType::SessionId:
		FindSessionById();
		break;
	case EKronosSpecificSessionQueryType::SessionOwnerId:
		; // intentional fall through
	default:
		FindOnlineSessions();
		break;
	}
}

bool UKronosMatchmakingSearchPass::FindOnlineSessions()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			UE_LOG(LogKronos, Log, TEXT("Finding online %s..."), *SessionName.ToString());

			AsyncStateFlags |= static_cast<uint8>(EKronosSearchPassAsyncStateFlags::FindingSessions);
			SearchState = EKronosSearchPassState::Searching;

			InitOnlineSessionSearch();

			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
			OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

			return SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
		}
	}

	OnFindOnlineSessionsComplete(false);
	return false;
}

void UKronosMatchmakingSearchPass::InitOnlineSessionSearch()
{
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = SearchParams.MaxSearchResults;
	SessionSearch->bIsLanQuery = SearchParams.bIsLanQuery;
	SessionSearch->TimeoutInSeconds = GetDefault<UKronosConfig>()->SearchTimeout;

	// ---- IMPORTANT! ----
	//
	// The Null subsystem doesn't support query settings! To combat this issue, session filtering has been introduced.
	// Query settings added via the ExtraQuerySettings array are filtered automatically.
	// However, additional query settings you set here have to be manually filtered in the FilterSearchResult() function!
	// --------------------

	// Filters for any search type:
	{
		SessionSearch->QuerySettings.Set(SETTING_SESSIONTYPE, SessionName.ToString(), EOnlineComparisonOp::Equals);
		SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, SearchParams.bSearchPresence, EOnlineComparisonOp::Equals);

		// SEARCH_LOBBIES was added in UE_4.27 as preparation for OSSv2.
		// So far only EOS uses it. Steam throws warnings so we'll only use it with EOS.
		{
			IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
			if (OnlineSubsystem && OnlineSubsystem->GetSubsystemName() == EOS_SUBSYSTEM)
			{
				SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, SearchParams.bSearchPresence, EOnlineComparisonOp::Equals);
			}
		}

		if (SearchParams.IgnoredSessions.Num() > 0)
		{
			// The expected format by the online subsystem is "uniqueid1;uniqueid2;uniqueid3".

			FString ExcludedUniqueIds;
			for (int32 Idx = 0; Idx < SearchParams.IgnoredSessions.Num(); Idx++)
			{
				const FUniqueNetIdRepl& IgnoredSession = SearchParams.IgnoredSessions[Idx];
				if (IgnoredSession.IsValid())
				{
					bool bAppendComma = Idx > 0; // Append comma before every unique id except the first
					ExcludedUniqueIds.Appendf(TEXT("%s%s"), bAppendComma ? TEXT(";") : TEXT(""), *IgnoredSession.ToString());
				}
			}

			SessionSearch->QuerySettings.Set(SEARCH_EXCLUDE_UNIQUEIDS, ExcludedUniqueIds, EOnlineComparisonOp::Equals);
		}
	}
	
	// Filters for specific session queries:
	if (SearchParams.IsSpecificSessionQuery())
	{
		// Only worry about session owner id based queries as other query types have dedicated search methods.
		if (SearchParams.SpecificSessionQuery.Type == EKronosSpecificSessionQueryType::SessionOwnerId)
		{
			FString OwnerId = SearchParams.SpecificSessionQuery.UniqueId.ToString();
			SessionSearch->QuerySettings.Set(SETTING_OWNERID, OwnerId, EOnlineComparisonOp::Equals);

			// Previously we also used the SEARCH_USER query setting but recently it's been causing issues.
			// If set the session wasn't being found with Steam, even though the Steam OSS doesn't have any code related to this.
			// Don't know why or how this affects the search. So for now we are not going to use it.
			//SessionSearch->QuerySettings.Set(SEARCH_USER, OwnerId, EOnlineComparisonOp::Equals);
		}
	}

	// Filters for regular searches:
	else
	{
		// Make sure that hidden sessions cannot be found.
		// This setting is an int32 because the Steam Subsystem doesn't support bool queries.
		SessionSearch->QuerySettings.Set(SETTING_HIDDEN, 0, EOnlineComparisonOp::Equals);

		if (!SearchParams.Playlist.IsEmpty())
		{
			SessionSearch->QuerySettings.Set(SETTING_PLAYLIST, SearchParams.Playlist, EOnlineComparisonOp::Equals);
		}

		if (!SearchParams.MapName.IsEmpty())
		{
			SessionSearch->QuerySettings.Set(SETTING_MAPNAME, SearchParams.MapName, EOnlineComparisonOp::Equals);
		}

		if (!SearchParams.GameMode.IsEmpty())
		{
			SessionSearch->QuerySettings.Set(SETTING_GAMEMODE, SearchParams.GameMode, EOnlineComparisonOp::Equals);
		}

		if (!SearchParams.bSkipEloChecks)
		{
			SessionSearch->QuerySettings.Set(SETTING_SESSIONELO, FMath::Max(SearchParams.Elo - SearchParams.EloRange, 0), EOnlineComparisonOp::GreaterThanEquals);
			SessionSearch->QuerySettings.Set(SETTING_SESSIONELO2, SearchParams.Elo + SearchParams.EloRange, EOnlineComparisonOp::LessThanEquals);
		}
	}

	// Register extra query settings.
	for (const FKronosQuerySetting& ExtraSetting : SearchParams.ExtraQuerySettings)
	{
		if (ExtraSetting.IsValid())
		{
			// Register the query setting the same way as SessionSearch->QuerySettings.Set(...) would
			{
				FOnlineSessionSearchParam* SearchParam = SessionSearch->QuerySettings.SearchParams.Find(ExtraSetting.Key);
				if (SearchParam)
				{
					SearchParam->Data = ExtraSetting.Data;
					SearchParam->ComparisonOp = ExtraSetting.ComparisonOp;
				}
				else
				{
					SessionSearch->QuerySettings.SearchParams.Add(ExtraSetting.Key, FOnlineSessionSearchParam(ExtraSetting.Data, ExtraSetting.ComparisonOp));
				}
			}
		}
	}
}

void UKronosMatchmakingSearchPass::OnFindOnlineSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogKronos, Log, TEXT("OnFindOnlineSessionsComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	AsyncStateFlags &= ~static_cast<uint8>(EKronosSearchPassAsyncStateFlags::FindingSessions);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		OnSearchComplete(SessionSearch->SearchResults);
		return;
	}

	SignalSearchPassComplete(EKronosSearchPassState::Failure, EKronosSearchPassCompleteResult::Failure);
}

bool UKronosMatchmakingSearchPass::FindFriendSession()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			UE_LOG(LogKronos, Log, TEXT("Finding friend %s..."), *SessionName.ToString());

			AsyncStateFlags |= static_cast<uint8>(EKronosSearchPassAsyncStateFlags::FindingSessions);
			SearchState = EKronosSearchPassState::Searching;

			SessionInterface->ClearOnFindFriendSessionCompleteDelegate_Handle(0, OnFindFriendSessionCompleteDelegateHandle);
			OnFindFriendSessionCompleteDelegateHandle = SessionInterface->AddOnFindFriendSessionCompleteDelegate_Handle(0, OnFindFriendSessionCompleteDelegate);

			return SessionInterface->FindFriendSession(0, *SearchParams.SpecificSessionQuery.UniqueId.GetUniqueNetId());
		}
	}

	OnFindFriendSessionComplete(0, false, TArray<FOnlineSessionSearchResult>());
	return false;
}

void UKronosMatchmakingSearchPass::OnFindFriendSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults)
{
	UE_LOG(LogKronos, Log, TEXT("OnFindFriendSessionComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	AsyncStateFlags &= ~static_cast<uint8>(EKronosSearchPassAsyncStateFlags::FindingSessions);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnFindFriendSessionCompleteDelegate_Handle(0, OnFindFriendSessionCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		OnSearchComplete(SearchResults);
		return;
	}

	RestartSearch();
	return;
}

bool UKronosMatchmakingSearchPass::FindSessionById()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			UE_LOG(LogKronos, Log, TEXT("Finding session by id %s..."), *SearchParams.SpecificSessionQuery.UniqueId.ToDebugString());

			AsyncStateFlags |= static_cast<uint8>(EKronosSearchPassAsyncStateFlags::FindingSessions);
			SearchState = EKronosSearchPassState::Searching;

			// FUniqueNetId constructor is protected, so we'll get an empty FUniqueNetIdRepl instead and get the unique id from that.
			static FUniqueNetIdRepl EmptyId = FUniqueNetIdRepl();

			const FUniqueNetIdPtr& PrimaryPlayerId = GetWorld()->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl().GetUniqueNetId();
			const FUniqueNetIdPtr& DesiredSessionId = SearchParams.SpecificSessionQuery.UniqueId.GetUniqueNetId();
			const FUniqueNetIdPtr& FriendId = EmptyId.GetUniqueNetId();

			return SessionInterface->FindSessionById(*PrimaryPlayerId, *DesiredSessionId, *FriendId, OnFindSessionByIdCompleteDelegate);
		}
	}

	OnFindSessionByIdComplete(0, false, FOnlineSessionSearchResult());
	return false;
}

void UKronosMatchmakingSearchPass::OnFindSessionByIdComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogKronos, Log, TEXT("OnFindSessionByIdComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	AsyncStateFlags &= ~static_cast<uint8>(EKronosSearchPassAsyncStateFlags::FindingSessions);

	if (bWasSuccessful)
	{
		TArray<FOnlineSessionSearchResult> SearchResults = TArray<FOnlineSessionSearchResult>();
		SearchResults.Add(SearchResult);

		OnSearchComplete(SearchResults);
		return;
	}

	RestartSearch();
	return;
}

void UKronosMatchmakingSearchPass::OnSearchComplete(const TArray<FOnlineSessionSearchResult>& InSearchResults)
{
	UE_LOG(LogKronos, Log, TEXT("Search complete. Sessions found: %d"), InSearchResults.Num());

	if (InSearchResults.Num() > 0)
	{
		// Filter out unwanted or invalid sessions.
		// This function doesn't contain any async tasks so it can be processed in one frame.
		// However nothing is stopping us from implementing an async state for this task if needed.
		FilterSearchResults(InSearchResults);

		UE_LOG(LogKronos, Log, TEXT("Filtering complete. Valid sessions: %d"), FilteredSessions.Num());

		// Begin pinging the remaining search results.
		// NOTE: Session pinging is not implemented for now.
		if (FilteredSessions.Num() > 0)
		{
			PingSearchResults();
			return;
		}
	}
	
	// No sessions were found, so we'll start a new search if possible.
	RestartSearch();
	return;
}

void UKronosMatchmakingSearchPass::FilterSearchResults(const TArray<FOnlineSessionSearchResult>& InSearchResults)
{
	UE_LOG(LogKronos, Log, TEXT("Filtering search results..."));

	FilteredSessions.Empty(InSearchResults.Num());

	if (InSearchResults.Num() > 0)
	{
		for (const FOnlineSessionSearchResult& SearchResult : InSearchResults)
		{
			if (FilterSearchResult(SearchResult))
			{
				// The session was deemed valid. Adding it to the filtered sessions list.
				FilteredSessions.Emplace(SearchResult);
			}
		}
	}
}

bool UKronosMatchmakingSearchPass::FilterSearchResult(const FOnlineSessionSearchResult& InSearchResult)
{
	UE_LOG(LogKronos, Verbose, TEXT("Filtering session: %s, Owner: %s"), *InSearchResult.GetSessionIdStr(), *InSearchResult.Session.OwningUserName);

	// Filters for any search type:
	{
		// Filter invalid sessions.
		if (!InSearchResult.IsValid())
		{
			UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - Session is invalid."));
			return false;
		}

		// Filter our own session.
		FUniqueNetIdPtr PrimaryPlayerId = GetWorld()->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl().GetUniqueNetId();
		if (InSearchResult.Session.OwningUserId == PrimaryPlayerId)
		{
			UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - Session is our own."));
			return false;
		}

		// Filter for session type.
		{
			FString SessionType;
			InSearchResult.Session.SessionSettings.Get(SETTING_SESSIONTYPE, SessionType);

			if (SessionType != SessionName.ToString())
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - SessionType didn't match '%s'."), *SessionName.ToString());
				return false;
			}
		}

		// Filter ignored sessions.
		{
			const FUniqueNetId& OwnerId = *InSearchResult.Session.OwningUserId;
			const FUniqueNetId& SessionId = InSearchResult.Session.SessionInfo->GetSessionId();

			if (SearchParams.IgnoredSessions.Contains(OwnerId) || SearchParams.IgnoredSessions.Contains(SessionId))
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - Session is in the ignored sessions list."));
				return false;
			}
		}
	}
	
	// Filters for specific session queries:
	if (SearchParams.IsSpecificSessionQuery())
	{
		// Filter for session owner id.
		if (SearchParams.SpecificSessionQuery.Type == EKronosSpecificSessionQueryType::SessionOwnerId)
		{
			FString OwnerId;
			InSearchResult.Session.SessionSettings.Get(SETTING_OWNERID, OwnerId);

			if (OwnerId != SearchParams.SpecificSessionQuery.UniqueId->ToString())
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - SessionOwnerId didn't match '%s'."), *SearchParams.SpecificSessionQuery.UniqueId.ToDebugString());
				return false;
			}
		}
	}

	// Filters for regular searches:
	else
	{
		// Filter hidden sessions.
		{
			bool SessionHidden;
			InSearchResult.Session.SessionSettings.Get(SETTING_HIDDEN, SessionHidden);

			if (SessionHidden)
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - Session is hidden."));
				return false;
			}
		}

		// Filter for session slots.
		if (SearchParams.MinSlotsRequired > 0)
		{
			if (InSearchResult.Session.NumOpenPublicConnections < SearchParams.MinSlotsRequired)
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - Not enough slots in session."));
				return false;
			}
		}

		// Filter for session playlist.
		if (!SearchParams.Playlist.IsEmpty())
		{
			FString SessionPlaylistName;
			InSearchResult.Session.SessionSettings.Get(SETTING_PLAYLIST, SessionPlaylistName);

			if (SessionPlaylistName != SearchParams.Playlist)
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - Playlist didn't match '%s'."), *SearchParams.Playlist);
				return false;
			}
		}

		// Filter for session map name.
		if (!SearchParams.MapName.IsEmpty())
		{
			FString SessionMapName;
			InSearchResult.Session.SessionSettings.Get(SETTING_MAPNAME, SessionMapName);

			if (SessionMapName != SearchParams.MapName)
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - MapName didn't match '%s'."), *SearchParams.MapName);
				return false;
			}
		}

		// Filter for session game mode.
		if (!SearchParams.GameMode.IsEmpty())
		{
			FString SessionGameMode;
			InSearchResult.Session.SessionSettings.Get(SETTING_GAMEMODE, SessionGameMode);

			if (SessionGameMode != SearchParams.GameMode)
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - GameMode didn't match '%s'."), *SearchParams.GameMode);
				return false;
			}
		}

		// Filter for session elo.
		if (!SearchParams.bSkipEloChecks)
		{
			int32 EloRangeMin = FMath::Max(SearchParams.Elo - SearchParams.EloRange, 0);
			int32 EloRangeMax = SearchParams.Elo + SearchParams.EloRange;

			int32 SessionElo;
			InSearchResult.Session.SessionSettings.Get(SETTING_SESSIONELO, SessionElo);

			int32 SessionElo2;
			InSearchResult.Session.SessionSettings.Get(SETTING_SESSIONELO2, SessionElo2);

			if (SessionElo < EloRangeMin)
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - SessionElo too low."));
				return false;
			}

			if (SessionElo2 > EloRangeMax)
			{
				UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - SessionElo too high."));
				return false;
			}
		}

		// Filter sessions which we or any of our party members are banned from.
		{
			FString BannedPlayers;
			InSearchResult.Session.SessionSettings.Get(SETTING_BANNEDPLAYERS, BannedPlayers);

			if (!BannedPlayers.IsEmpty())
			{
				TArray<FString> BannedPlayersArray;
				if (BannedPlayers.ParseIntoArray(BannedPlayersArray, TEXT(";")) > 0)
				{
					UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
					if (PartyManager->IsPartyLeader())
					{
						for (FUniqueNetIdRepl& PartyPlayerUniqueId : PartyManager->GetPartyPlayerUniqueIds())
						{
							if (BannedPlayersArray.Contains(PartyPlayerUniqueId.ToString()))
							{
								UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - A party member is banned from the session."));
								return false;
							}
						}
					}

					else
					{
						FUniqueNetIdPtr PrimaryPlayerId = GetWorld()->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl().GetUniqueNetId();
						if (BannedPlayersArray.Contains(PrimaryPlayerId->ToString()))
						{
							UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - The player is banned from the session."));
							return false;
						}
					}
				}
			}
		}
	}

	// Filter extra query settings.
	for (FKronosQuerySetting& ExtraQuery : SearchParams.ExtraQuerySettings)
	{
		const FOnlineSessionSetting* Setting = InSearchResult.Session.SessionSettings.Settings.Find(ExtraQuery.Key);
		if (!Setting)
		{
			UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - %s extra query setting has no corresponding session setting on the session."), *ExtraQuery.Key.ToString());
			return false;
		}

		bool bValid = false;

		switch (ExtraQuery.Data.GetType())
		{
		case EOnlineKeyValuePairDataType::Int32:
			bValid = ExtraQuery.CompareAgainst<int32>(Setting);
			break;
		case EOnlineKeyValuePairDataType::String:
			bValid = ExtraQuery.CompareAgainst<FString>(Setting);
			break;
		case EOnlineKeyValuePairDataType::Float:
			bValid = ExtraQuery.CompareAgainst<float>(Setting);
			break;
		case EOnlineKeyValuePairDataType::Bool:
			bValid = ExtraQuery.CompareAgainst<bool>(Setting);
			break;
		default:
			UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - %s extra query setting has an invalid value type."), *ExtraQuery.Key.ToString());
			return false;
		}

		if (!bValid)
		{
			UE_LOG(LogKronos, Verbose, TEXT("Result: Invalid - %s extra query setting auto-comparison returned false."), *ExtraQuery.Key.ToString());
			return false;
		}
	}

	return true;
}

void UKronosMatchmakingSearchPass::PingSearchResults()
{
	UE_LOG(LogKronos, Log, TEXT("Pinging search results..."));

	AsyncStateFlags |= static_cast<uint8>(EKronosSearchPassAsyncStateFlags::PingingSessions);
	SearchState = EKronosSearchPassState::PingingSessions;

	UE_LOG(LogKronos, Log, TEXT("KronosMatchmakingSearchPass::PingSearchResults - not implemented"));

	OnPingSearchResultsComplete(false);
}

void UKronosMatchmakingSearchPass::OnPingSearchResultsComplete(bool bWasSuccessful)
{
	UE_LOG(LogKronos, Log, TEXT("OnPingSearchResultsComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	AsyncStateFlags &= ~static_cast<uint8>(EKronosSearchPassAsyncStateFlags::PingingSessions);

	if (bWasCanceled)
	{
		SignalCancelSearchPassCompleteChecked();
		return;
	}

	// Give us a chance to sort search results (e.g. prefer sessions with good ping).
	SortSearchResults();

	UE_LOG(LogKronos, Log, TEXT("Sorting complete."));
	SignalSearchPassComplete(EKronosSearchPassState::Complete, EKronosSearchPassCompleteResult::Success);
}

void UKronosMatchmakingSearchPass::SortSearchResults()
{
	UE_LOG(LogKronos, Log, TEXT("KronosMatchmakingSearchPass::SortSearchResults - not implemented"));
}

void UKronosMatchmakingSearchPass::RestartSearch()
{
	if (CurrentAttemptIdx < SearchParams.MaxSearchAttempts)
	{
		KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosMatchmakingSearchPass: Restarting search (%d/%d)..."), CurrentAttemptIdx + 1, SearchParams.MaxSearchAttempts);

		FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this]()
		{
			BeginSearchAttempt();
		});

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_SearchDelay, TimerDelegate, GetDefault<UKronosConfig>()->RestartSearchPassDelay, false);
		return;
	}

	SignalSearchPassComplete(EKronosSearchPassState::Complete, EKronosSearchPassCompleteResult::NoSession);
}

bool UKronosMatchmakingSearchPass::CancelFindSessions()
{
	UE_LOG(LogKronos, Log, TEXT("KronosMatchmakingSearchPass: Canceling find sessions..."));

	AsyncStateFlags |= static_cast<uint8>(EKronosSearchPassAsyncStateFlags::CancelingSearch);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnCancelFindSessionsCompleteDelegate_Handle(OnCancelFindSessionsCompleteDelegateHandle);
			OnCancelFindSessionsCompleteDelegateHandle = SessionInterface->AddOnCancelFindSessionsCompleteDelegate_Handle(OnCancelFindSessionsCompleteDelegate);

			return SessionInterface->CancelFindSessions();
		}
	}

	OnCancelFindSessionsComplete(false);
	return false;
}

void UKronosMatchmakingSearchPass::OnCancelFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogKronos, Log, TEXT("OnCancelFindSessionsComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	AsyncStateFlags &= ~static_cast<uint8>(EKronosSearchPassAsyncStateFlags::CancelingSearch);
	AsyncStateFlags &= ~static_cast<uint8>(EKronosSearchPassAsyncStateFlags::FindingSessions);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// Note that we are clearing the find session delegates as well!
			SessionInterface->ClearOnCancelFindSessionsCompleteDelegate_Handle(OnCancelFindSessionsCompleteDelegateHandle);
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
			SessionInterface->ClearOnFindFriendSessionCompleteDelegate_Handle(0, OnFindFriendSessionCompleteDelegateHandle);
		}
	}

	SignalCancelSearchPassCompleteChecked();
}

void UKronosMatchmakingSearchPass::SignalSearchPassComplete(const EKronosSearchPassState EndState, const EKronosSearchPassCompleteResult Result)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosMatchmakingSearchPass: OnSearchPassComplete with result: %s"), LexToString(Result));

	SearchState = EndState;
	OnSearchPassComplete().ExecuteIfBound(SessionName, Result);
}

void UKronosMatchmakingSearchPass::SignalCancelSearchPassComplete()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosMatchmakingSearchPass: OnCancelSearchPassComplete with result: Success"));

	SearchState = EKronosSearchPassState::Canceled;
	OnCancelSearchPassComplete().ExecuteIfBound();
}

bool UKronosMatchmakingSearchPass::SignalCancelSearchPassCompleteChecked()
{
	if (bWasCanceled && AsyncStateFlags == 0 && SearchState != EKronosSearchPassState::Canceled)
	{
		SignalCancelSearchPassComplete();
		return true;
	}

	return false;
}

void UKronosMatchmakingSearchPass::DumpSettings() const
{
	UE_LOG(LogKronos, Log, TEXT("Dumping search pass settings..."));
	UE_LOG(LogKronos, Log, TEXT("  SessionName: %s"), *SessionName.ToString());
	UE_LOG(LogKronos, Log, TEXT("  Params:"));
	UE_LOG(LogKronos, Log, TEXT("    Playlist: %s"), *SearchParams.Playlist);
	UE_LOG(LogKronos, Log, TEXT("    MapName: %s"), *SearchParams.MapName);
	UE_LOG(LogKronos, Log, TEXT("    GameMode: %s"), *SearchParams.GameMode);
	UE_LOG(LogKronos, Log, TEXT("    MaxSearchAttempts: %d"), SearchParams.MaxSearchAttempts);
	UE_LOG(LogKronos, Log, TEXT("    MaxSearchResults: %d"), SearchParams.MaxSearchResults);
	UE_LOG(LogKronos, Log, TEXT("    MinSlotsRequired: %d"), SearchParams.MinSlotsRequired);
	UE_LOG(LogKronos, Log, TEXT("    Elo: %d"), SearchParams.Elo);
	UE_LOG(LogKronos, Log, TEXT("    EloRange: %d"), SearchParams.EloRange);
	UE_LOG(LogKronos, Log, TEXT("    bIsLanQuery: %s"), SearchParams.bIsLanQuery ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("    bSearchPresence: %s"), SearchParams.bSearchPresence ? TEXT("True") : TEXT("False"));
	UE_LOG(LogKronos, Log, TEXT("    bSkipEloChecks: %s"), SearchParams.bSkipEloChecks ? TEXT("True") : TEXT("False"));

	FString SpecificSessionQueryString = FString::Printf(TEXT("[%s] %s"), LexToString(SearchParams.SpecificSessionQuery.Type), *SearchParams.SpecificSessionQuery.UniqueId.ToDebugString());
	UE_LOG(LogKronos, Log, TEXT("    SpecificSessionQuery: %s"), SearchParams.IsSpecificSessionQuery() ? *SpecificSessionQueryString : TEXT("-"));

	UE_LOG(LogKronos, Log, TEXT("    ExtraQuerySettings: %s"), SearchParams.ExtraQuerySettings.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FKronosQuerySetting& ExtraSetting : SearchParams.ExtraQuerySettings)
	{
		UE_LOG(LogKronos, Log, TEXT("      %s=%s (%s)"), *ExtraSetting.Key.ToString(), *ExtraSetting.Data.ToString(), EOnlineComparisonOp::ToString(ExtraSetting.ComparisonOp));
	}

	UE_LOG(LogKronos, Log, TEXT("    IgnoredSessions: %s"), SearchParams.IgnoredSessions.Num() > 0 ? TEXT("") : TEXT("-"));
	for (const FUniqueNetIdRepl& IgnoredSession : SearchParams.IgnoredSessions)
	{
		UE_LOG(LogKronos, Log, TEXT("      %s"), *IgnoredSession.ToDebugString());
	}
}

void UKronosMatchmakingSearchPass::DumpFilteredSessions() const
{
	UE_LOG(LogKronos, Log, TEXT("Dumping filtered sessions:"), FilteredSessions.Num());
	if (FilteredSessions.Num() > 0)
	{
		for (int32 Idx = 0; Idx < FilteredSessions.Num(); Idx++)
		{
			const FKronosSearchResult& SearchResult = FilteredSessions[Idx];
			if (SearchResult.OnlineResult.IsValid())
			{
				UE_LOG(LogKronos, Log, TEXT("  %d. %s %s"), Idx, *SearchResult.GetSessionType().ToString(), *SearchResult.OnlineResult.GetSessionIdStr());
			}

			else UE_LOG(LogKronos, Log, TEXT("  %d. %s"), Idx, TEXT("INVALID"));
		}
	}

	else UE_LOG(LogKronos, Log, TEXT("  Empty."));
}
