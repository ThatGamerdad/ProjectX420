// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KronosTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "KronosMatchmakingSearchPass.generated.h"

/**
 * Delegate triggered when search pass is complete.
 *
 * @param SessionName The name of the session that we've been matchmaking for.
 * @param Result State the search pass ended with.
 */
DECLARE_DELEGATE_TwoParams(FOnMatchmakingSearchPassComplete, const FName /** SessionName */, const EKronosSearchPassCompleteResult /** Result */);

/**
 * Delegate triggered when search pass is canceled.
 */
DECLARE_DELEGATE(FOnCancelMatchmakingSearchPassComplete);

/**
 * KronosMatchmakingSearchPass is responsible for handling session search requests for the associated KronosMatchmakingPolicy object.
 * It implements and executes a chain of functions to build a search flow. This search flow from start to finish is known as a search pass.
 *
 * Even though this class is designed to be completely independent, it never exists on its own.
 * It is always spawned and managed by a matchmaking policy object.
 *
 * @see UKronosMatchmakingPolicy
 */
UCLASS(notplaceable, Within = KronosMatchmakingPolicy)
class KRONOS_API UKronosMatchmakingSearchPass : public UObject
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UKronosMatchmakingSearchPass(const FObjectInitializer& ObjectInitializer);

public:

	/** Name of the session acted upon. */
	UPROPERTY(Transient)
	FName SessionName;

	/** Search params. */
	UPROPERTY(Transient)
	FKronosSearchParams SearchParams;

	/** Current state of the search pass object. */
	EKronosSearchPassState SearchState;

protected:

	/** Has search pass been marked to cancel. */
	bool bWasCanceled;

	/** Session search that is being processed by the online subsystem. */
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	/** Sessions found by the search pass. Only valid after a successful search. */
	TArray<FKronosSearchResult> FilteredSessions;

	/** Handle used to delay search attempts. */
	FTimerHandle TimerHandle_SearchDelay;

	/** The index of the current search attempt. */
	int32 CurrentAttemptIdx;

	/** Active async state(s) of the search pass. See EKronosSearchPassAsyncStateFlags for possible states. */
	uint8 AsyncStateFlags;

protected:

	/** Delegate triggered when the search for online sessions is complete. */
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;

	/** Delegate triggered when the search for a friends session is complete. */
	FOnFindFriendSessionCompleteDelegate OnFindFriendSessionCompleteDelegate;

	/** Delegate triggered when the search for a session by id is complete. */
	FOnSingleSessionResultCompleteDelegate OnFindSessionByIdCompleteDelegate;

	/** Delegate triggered when a search request is canceled. */
	FOnCancelFindSessionsCompleteDelegate OnCancelFindSessionsCompleteDelegate;

	/** Handle for find online sessions completion delegate. */
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;

	/** Handle for find friend session completion delegate. */
	FDelegateHandle OnFindFriendSessionCompleteDelegateHandle;

	/** Handle for cancel find sessions completion delegate. */
	FDelegateHandle OnCancelFindSessionsCompleteDelegateHandle;

private:

	/** Delegate triggered when search pass is complete. */
	FOnMatchmakingSearchPassComplete MatchmakingSearchPassComplete;

	/** Delegate triggered when search pass is canceled. */
	FOnCancelMatchmakingSearchPassComplete CancelMatchmakingSearchPassComplete;

public:

	/**
	 * Start a new search pass.
	 *
	 * @param InSessionName Name of the session to matchmaking for. Should be either NAME_GameSession or NAME_PartySession!
	 * @param InParams Search params. Check out FKronosSearchParams for list of params.
	 */
	virtual bool StartSearch(const FName InSessionName, const FKronosSearchParams& InParams);

	/** Cancel search pass. */
	virtual bool CancelSearch();

	/** Is search in progress. */
	virtual bool IsSearching() const;

	/**
	 * Get a single search result from the filtered sessions array at the given index.
	 *
	 * @param InSessionIdx Session to get.
	 * @param OutSearchResult The session from the array. Only valid if the function returned true!.
	 *
	 * @return True if session found. False otherwise.
	 */
	virtual bool GetSearchResult(const int32 InSessionIdx, FKronosSearchResult& OutSearchResult);

	/** Get all filtered sessions. */
	virtual TArray<FKronosSearchResult>& GetSearchResults() { return FilteredSessions; }

	/** Clears all timers and delegates. Called by the associated matchmaking policy object when it is getting invalidated. */
	virtual void Invalidate();

	/** Dumps the configurations of the search pass to the console. */
	virtual void DumpSettings() const;

	/** Dumps filtered sessions into the console. */
	virtual void DumpFilteredSessions() const;

	/** @return The delegate fired when search pass is complete. */
	FOnMatchmakingSearchPassComplete& OnSearchPassComplete() { return MatchmakingSearchPassComplete; }

	/** @return The delegate fired when search pass is canceled. */
	FOnCancelMatchmakingSearchPassComplete& OnCancelSearchPassComplete() { return CancelMatchmakingSearchPassComplete; }

protected:

	/** Starts a new search attempt. */
	virtual void BeginSearchAttempt();

	/** Finds sessions using a regular search. */
	virtual bool FindOnlineSessions();

	/** Search settings to be used when finding sessions. */
	virtual void InitOnlineSessionSearch();

	/** Entry point after session search request has completed. */
	virtual void OnFindOnlineSessionsComplete(bool bWasSuccessful);

	/** Finds a specific session by the given friend id. */
	virtual bool FindFriendSession();

	/** Entry point after finding a specific session via a friend id is complete. */
	virtual void OnFindFriendSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults);

	/** Finds a specific session by the given id.*/
	virtual bool FindSessionById();

	/** Entry point after finding a specific session via session id is complete. */
	virtual void OnFindSessionByIdComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult);

protected:

	/** Entry point after a search is complete. */
	virtual void OnSearchComplete(const TArray<FOnlineSessionSearchResult>& InSearchResults);

	/** Filter search results. */
	virtual void FilterSearchResults(const TArray<FOnlineSessionSearchResult>& InSearchResults);

	/** Filter the given search result. */
	virtual bool FilterSearchResult(const FOnlineSessionSearchResult& InSearchResult);

	/** Ping filtered search results. */
	virtual void PingSearchResults();

	/** Entry point after pinging search results is complete. */
	virtual void OnPingSearchResultsComplete(bool bWasSuccessful);

	/** Sort filtered search results. */
	virtual void SortSearchResults();

	/** Start a new search pass if there are search attempts left. Otherwise completion is signaled. */
	virtual void RestartSearch();

	/** Cancel any active session search requests. */
	virtual bool CancelFindSessions();

	/** Entry point after canceling session search request is complete. */
	virtual void OnCancelFindSessionsComplete(bool bWasSuccessful);

	/** Triggers search pass complete delegate. */
	virtual void SignalSearchPassComplete(const EKronosSearchPassState EndState, const EKronosSearchPassCompleteResult Result);

	/** Triggers search pass canceled delegate. */
	virtual void SignalCancelSearchPassComplete();

	/** Triggers search pass canceled delegate if no async tasks left, and the search pass hasn't been canceled yet. */
	virtual bool SignalCancelSearchPassCompleteChecked();
};
