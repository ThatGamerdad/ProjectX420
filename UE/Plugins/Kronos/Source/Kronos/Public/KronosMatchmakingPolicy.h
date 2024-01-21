// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KronosTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "KronosMatchmakingPolicy.generated.h"

/**
 * Delegate triggered when matchmaking is started.
 */
DECLARE_EVENT(UKronosMatchmakingPolicy, FOnStartKronosMatchmakingComplete);

/**
 * Delegate triggered when matchmaking is complete.
 *
 * @param SessionName The name of the session that we've been matchmaking for.
 * @param Result State the matchmaking ended with.
 */
DECLARE_EVENT_TwoParams(UKronosMatchmakingPolicy, FOnKronosMatchmakingComplete, const FName /** SessionName */, const EKronosMatchmakingCompleteResult /** Result */);

/**
 * Delegate triggered when matchmaking is canceled.
 */
DECLARE_EVENT(UKronosMatchmakingPolicy, FOnCancelKronosMatchmakingComplete);

/**
 * Delegate triggered when matchmaking state changes.
 *
 * @param OldState The state we are leaving.
 * @param NewState The state we are entering.
 */
DECLARE_EVENT_TwoParams(UKronosMatchmakingPolicy, FOnKronosMatchmakingStateChanged, const EKronosMatchmakingState /** OldState */, const EKronosMatchmakingState /** NewState */);

/**
 * Delegate triggered when either the matchmaking state or time changes. Helper delegate for UI elements.
 *
 * @param MatchmakingState Current state of the matchmaking.
 * @param MatchmakingTime Elapsed time since matchmaking started.
 */
DECLARE_EVENT_TwoParams(UKronosMatchmakingPolicy, FOnKronosMatchmakingUpdated, const EKronosMatchmakingState /** MatchmakingState */, const int32 /** MatchmakingTime */);

/**
 * Delegate triggered when a cleanup task is complete in the matchmaking pass.
 */
DECLARE_DELEGATE_OneParam(FOnCleanupKronosMatchmakingComplete, bool /** bWasSuccessful */);

/**
 * KronosMatchmakingPolicy acts as a manager for the matchmaking. It implements and executes a chain of functions to build a matchmaking flow.
 * A matchmaking flow from start to finish is known as a matchmaking pass.
 *
 * Each matchmaking policy will have a dedicated KronosMatchmakingSearchPass object associated with it.
 * This object will be responsible for finding and evaluating session search results.
 *
 * The matchmaking policy is also responsible for requesting reservations with sessions.
 * To achieve this, a reservation client beacon will be managed by the matchmaking policy when requesting a reservation.
 *
 * Keep in mind that only one matchmaking policy should be active at any time, and that each matchmaking policy can only be started once.
 * If you want to restart matchmaking with different params, you should create a new matchmaking policy.
 *
 * @see UKronosMatchmakingSearchPass
 * @see UKronosMatchmakingManager
 */
UCLASS(notplaceable, Within = Object)
class KRONOS_API UKronosMatchmakingPolicy : public UObject
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UKronosMatchmakingPolicy(const FObjectInitializer& ObjectInitializer);

public:

	/** Name of the session acted upon. */
	FName SessionName;

	/** Matchmaking params. */
	FKronosMatchmakingParams MatchmakingParams;

	/** Matchmaking flags. */
	uint8 MatchmakingFlags;

	/** Session to join when using 'JoinOnly' matchmaking mode. */
	FKronosSearchResult SessionToJoin;

	/** Matchmaking mode. */
	EKronosMatchmakingMode MatchmakingMode;

protected:

	/** Has matchmaking been started. */
	bool bWasStarted;

	/** Has matchmaking been marked to cancel. If true, the matchmaking is either being canceled, or canceled already. */
	bool bWasCanceled;

	/** Is matchmaking in progress. */
	bool bMatchmakingInProgress;

	/** Current state of the matchmaking. */
	EKronosMatchmakingState MatchmakingState;

	/** The elapsed time in seconds since matchmaking started. */
	int32 MatchmakingTime;

	/** Handle used when the matchmaking process doesn't want to go into the next state immediately. */
	FTimerHandle TimerHandle_MatchmakingDelay;

	/** Handle used to tick the elapsed time. Runs on a one sec tick. */
	FTimerHandle TimerHandle_MatchmakingTimer;

	/** The search pass object associated with the matchmaking policy. */
	UPROPERTY(Transient)
	class UKronosMatchmakingSearchPass* SearchPass;

	/** The reservation client that is handling a reservation requests. Only valid while we have a pending reservation. */
	UPROPERTY(Transient)
	class AKronosReservationClient* ReservationBeaconClient;

	/** Active async state(s) of the matchmaking policy. See EKronosMatchmakingAsyncStateFlags for possible states. */
	uint8 MatchmakingAsyncStateFlags;

	/** The index of the current matchmaking pass. Basically, 1 + how many times have we hit a matchmaking end point (e.g. zero sessions found). */
	int32 CurrentMatchmakingPassIdx;

	/** The index of the search result that is currently being tested. */
	int32 CurrentSessionIdx;

	/** The result of the matchmaking. Only valid after the matchmaking has been completed. */
	EKronosMatchmakingCompleteResult MatchmakingResult;

	/** Reason behind the failure. Only valid after matchmaking has resulted in failure. */
	EKronosMatchmakingFailureReason FailureReason;

protected:

	/** Delegate triggered when a session create request is complete with the online subsystem. */
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;

	/** Delegate triggered when a session join request is complete with the online subsystem. */
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;

	/** Handle for session create request complete delegate. */
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;

	/** Handle for session join request complete delegate. */
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;

private:

	/** Delegate triggered when matchmaking is started. */
	FOnStartKronosMatchmakingComplete StartKronosMatchmakingComplete;

	/** Delegate triggered when matchmaking is complete. */
	FOnKronosMatchmakingComplete KronosMatchmakingComplete;

	/** Delegate triggered when matchmaking is canceled. */
	FOnCancelKronosMatchmakingComplete CancelKronosMatchmakingComplete;

	/** Delegate triggered when matchmaking state changes. */
	FOnKronosMatchmakingStateChanged KronosMatchmakingStateChanged;

	/** Delegate triggered when either the matchmaking state or time changes. Helper delegate for UI elements. */
	FOnKronosMatchmakingUpdated KronosMatchmakingUpdated;

public:

	/**
	 * Start the matchmaking.
	 *
	 * Each matchmaking policy is supposed to be started only once.
	 * If you want to restart the matchmaking with different params, you should create a new matchmaking policy.
	 *
	 * @param InSessionName Name of the session to matchmaking for. Should be either NAME_GameSession or NAME_PartySession!
	 * @param InParams Matchmaking params. Check out FKronosMatchmakingParams for list of params.
	 * @param InFlags Matchmaking flags. Check out EKronosMatchmakingFlags for possible flags.
	 * @param InMode Matchmaking mode. Check out EKronosMatchmakingMode for possible modes.
	 * @param InStartDelay Delay in seconds before actually starting the matchmaking.
	 * @param InSessionToJoin Session to join. Only relevant if the matchmaking mode is set to EKronosMatchmakingMode::JoinOnly.
	 */
	virtual void StartMatchmaking(const FName InSessionName, const FKronosMatchmakingParams& InParams, const uint8 InFlags = 0, const EKronosMatchmakingMode InMode = EKronosMatchmakingMode::Default, const float InStartDelay = 0.0f, const FKronosSearchResult& InSessionToJoin = FKronosSearchResult());

	/**
	 * Cancel matchmaking if in progress.
	 */
	virtual void CancelMatchmaking();

	/**
	 * Is matchmaking in progress.
	 *
	 * @return True if the policy object is processing (including the canceling state!). False otherwise.
	 */
	virtual bool IsMatchmaking() const { return bMatchmakingInProgress; }

	/** Clears all timers and delegates on both the matchmaking policy, and the corresponding search pass and reservation client objects. */
	virtual void Invalidate();

	/** Dumps the matchmaking configuration to the console. */
	virtual void DumpSettings() const;

	/** Dumps the matchmaking state to the console. */
	virtual void DumpMatchmakingState() const;

	/** @return The associated search pass object. */
	UKronosMatchmakingSearchPass* GetSearchPass() const { return SearchPass; }

	/** @return The current matchmaking state. */
	EKronosMatchmakingState GetMatchmakingState() const { return MatchmakingState; }

	/** @return The result of the matchmaking. Only valid after the matchmaking has been completed. */
	EKronosMatchmakingCompleteResult GetMatchmakingResult() const { return MatchmakingResult; }

	/** @return The reason behind the failure. Only valid after matchmaking has resulted in failure. */
	EKronosMatchmakingFailureReason GetFailureReason() const { return FailureReason; }

	/** @return The delegate fired when matchmaking is started. */
	FOnStartKronosMatchmakingComplete& OnStartKronosMatchmakingComplete() { return StartKronosMatchmakingComplete; }

	/** @return The delegate fired when matchmaking is complete. */
	FOnKronosMatchmakingComplete& OnKronosMatchmakingComplete() { return KronosMatchmakingComplete; }

	/** @return The delegate fired when matchmaking is canceled. */
	FOnCancelKronosMatchmakingComplete& OnCancelKronosMatchmakingComplete() { return CancelKronosMatchmakingComplete; }

	/** @return The delegate fired when matchmaking state changes. */
	FOnKronosMatchmakingStateChanged& OnKronosMatchmakingStateChanged() { return KronosMatchmakingStateChanged; }

	/** @return The delegate fired when either the matchmaking state or time changes. This is a helper delegate for UI elements. */
	FOnKronosMatchmakingUpdated& OnKronosMatchmakingUpdated() { return KronosMatchmakingUpdated; }

protected:

	/** Starts the matchmaking in the selected matchmaking mode. */
	virtual void BeginMatchmaking();

	/** Entry point for matchmaking mode: EKronosMatchmakingMode::Default */
	virtual void HandleStartingDefaultMatchmaking();

	/** Entry point for matchmaking mode: EKronosMatchmakingMode::CreateOnly */
	virtual void HandleStartingCreateOnlyMatchmaking();

	/** Entry point for matchmaking mode: EKronosMatchmakingMode::SearchOnly */
	virtual void HandleStartingSearchOnlyMatchmaking();

	/** Entry point for matchmaking mode: EKronosMatchmakingMode::JoinOnly */
	virtual void HandleStartingJoinOnlyMatchmaking();

	/** Starts a new search pass. Called when matchmaking is started, or after a matchmaking pass when restarting. */
	virtual void StartSearchPass();

	/**
	 * Calculate the Elo range to be used for the given matchmaking pass.
	 * Called when matchmaking is initially started, or when restarting matchmaking after a matchmaking pass.
	 * You can override this function to implement your own algorith.
	 */
	virtual int32 GetEloSearchRangeFor(int32 MatchmakingPassIdx);

	/** Entry point after a search pass is complete. */
	virtual void OnSearchPassComplete(const FName InSessionName, const EKronosSearchPassCompleteResult Result);

	/** Entry point after a search pass is canceled. */
	virtual void OnCancelSearchPassComplete();

	/** Called after a successful search pass. Starts reserving / joining search results one by one until a success is received, or we run out of search results. */
	virtual void StartTestingSearchResults();

	/** Return point after each failed reservation / join attempt. */
	virtual void ContinueTestingSearchResults();

	/** Starts a new matchmaking pass. Called after the previous matchmaking pass exhausted all options. */
	virtual void RestartMatchmaking();

	/** Changes matchmaking state. Also triggers matchmaking state changed delegate (if the state actually changed). */
	virtual void SetMatchmakingState(const EKronosMatchmakingState InState);

	/** Triggers matchmaking started delegate. */
	virtual void SignalStartMatchmakingComplete();

	/** Triggers matchmaking complete delegate. */
	virtual void SignalMatchmakingComplete(const EKronosMatchmakingState EndState, const EKronosMatchmakingCompleteResult Result, const EKronosMatchmakingFailureReason Reason = EKronosMatchmakingFailureReason::Unknown);

	/** Triggers matchmaking canceled delegate. */
	virtual void SignalCancelMatchmakingComplete();

	/** Triggers matchmaking complete delegate if no async tasks left, and matchmaking hasn't been canceled yet. */
	virtual bool SignalCancelMatchmakingCompleteChecked();

protected:

	/** Starts creating a session through the online subsystem. */
	virtual bool CreateOnlineSession();

	/** @return Session settings to be used when creating a session. */
	virtual FOnlineSessionSettings InitOnlineSessionSettings();

	/** @return Beacon port to be used when creating a session. */
	virtual int32 GetPreferredBeaconPort();

	/** @return Server name to be used when no server name was given via matchmaking params when creating a session. */
	virtual FString GetDefaultServerName();

	/** Entry point after session create request has completed. */
	virtual void OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful);

	/** Starts requesting a reservation with the given session. */
	virtual bool RequestReservation(const FKronosSearchResult& InSession);

	/** Entry point after a reservation request is completed. */
	virtual void OnRequestReservationComplete(const FKronosSearchResult& SearchResult, const EKronosReservationCompleteResult Result);

	/** Starts joining the session through the online subsystem. */
	virtual bool JoinOnlineSession(const FKronosSearchResult& InSession);

	/** Entry point after session join request has completed. */
	virtual void OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);

protected:

	/** Cleans up the given online session before continuing. */
	virtual bool CleanupExistingSession(const FName InSessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate());

	/** Cleans up the current pending reservation before continuing. */
	virtual bool CleanupExistingReservations(const FOnCleanupKronosMatchmakingComplete& CompletionDelegate = FOnCleanupKronosMatchmakingComplete());

protected:

	/** @return The class to be used when creating the associated search pass object. */
	virtual TSubclassOf<UKronosMatchmakingSearchPass> GetSearchPassClass();

	/** @return The class to be used when creating a reservation client. */
	virtual TSubclassOf<AKronosReservationClient> GetReservationClientClass();

public:

	/** @return Debug data to be displayed in the Gameplay Debugger. */
	virtual FString GetDebugString() const;
};
