// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KronosTypes.h"
#include "KronosMatchmakingManager.generated.h"

class UKronosMatchmakingPolicy;

/**
 * Delegate triggered when a new matchmaking policy object is created and registered.
 *
 * @param MatchmakingPolicy The matchmaking policy that was created.
 */
DECLARE_DELEGATE_OneParam(FOnCreateMatchmakingPolicyComplete, UKronosMatchmakingPolicy* /** MatchmakingPolicy */);

/** Blueprint friendly versions of the matchmaking delegates. Duplicated from UKronosMatchmakingPolicy. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FK2_OnStartKronosMatchmakingComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FK2_OnKronosMatchmakingComplete, const FName, SessionName, const EKronosMatchmakingCompleteResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FK2_OnCancelKronosMatchmakingComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FK2_OnKronosMatchmakingStateChanged, const EKronosMatchmakingState, OldState, const EKronosMatchmakingState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FK2_OnKronosMatchmakingUpdated, const EKronosMatchmakingState, MatchmakingState, const int32, MatchmakingTime);

/**
 * KronosMatchmakingManager is responsible for ensuring that only one matchmaking policy is active at any time.
 * It is automatically spawned and managed by the KronosOnlineSession class.
 * 
 * @see UKronosMatchmakingPolicy
 */
UCLASS(Blueprintable, BlueprintType)
class KRONOS_API UKronosMatchmakingManager : public UObject
{
	GENERATED_BODY()

protected:

	/**
	 * The currently active matchmaking policy object.
	 * This object remains valid even after matchmaking is complete so that we can read into matchmaking results.
	 */
	UPROPERTY(Transient)
	UKronosMatchmakingPolicy* MatchmakingPolicy;

private:

	/** Event when matchmaking is started. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FK2_OnStartKronosMatchmakingComplete OnMatchmakingStartedEvent;

	/** Event when matchmaking is complete. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FK2_OnKronosMatchmakingComplete OnMatchmakingCompleteEvent;

	/** Event when matchmaking is canceled. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FK2_OnCancelKronosMatchmakingComplete OnMatchmakingCanceledEvent;

	/** Event when matchmaking state changes. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FK2_OnKronosMatchmakingStateChanged OnMatchmakingStateChangedEvent;

	/** Event when either the matchmaking state or time changes. Helper delegate for UI elements. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FK2_OnKronosMatchmakingUpdated OnMatchmakingUpdatedEvent;

public:

	/** Get the matchmaking manager from the KronosOnlineSession. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Get Kronos Matchmaking Manager", meta = (WorldContext = "WorldContextObject"))
	static UKronosMatchmakingManager* Get(const UObject* WorldContextObject);

	/**
	 * Initialize the KronosMatchmakingManager during game startup.
	 * Called by the KronosOnlineSession.
	 */
	virtual void Initialize() {};

	/**
	 * Deinitialize the KronosMatchmakingManager before game shutdown.
	 * Called by the KronosOnlineSession.
	 */
	virtual void Deinitialize() {};

public:

	/**
	 * Creates a new matchmaking policy. If matchmaking is in progress, it will be canceled first.
	 * NOTE: This operation is async.
	 *
	 * @param CompletionDelegate Delegate called when the matchmaking policy is created.
	 * @param bBindGlobalDelegates Whether to bind global matchmaking delegates in the matchmaking manager.
	 * @param bAutoHandleCompletion Should Kronos automatically handle matchmaking complete events in KronosOnlineSession class.
	 */
	virtual void CreateMatchmakingPolicy(const FOnCreateMatchmakingPolicyComplete& CompletionDelegate = FOnCreateMatchmakingPolicyComplete(), bool bBindGlobalDelegates = true, bool bAutoHandleCompletion = true);

	/**
	 * Register the matchmaking policy as the currently active matchmaking.
	 * At this point no checks will be done. The current matchmaking policy (if there is one) will simply be overridden.
	 *
	 * @param InPolicy The matchmaking policy to register.
	 * @param bBindGlobalDelegates Whether to bind global matchmaking delegates in the matchmaking manager.
	 * @param bAutoHandleCompletion Should Kronos automatically handle matchmaking complete events in KronosOnlineSession class.
	 */
	virtual void RegisterMatchmakingPolicy(UKronosMatchmakingPolicy* InPolicy, bool bBindGlobalDelegates, bool bAutoHandleCompletion);

	/** @return Whether matchmaking is in progress or not. */
	virtual bool IsMatchmaking() const;

	/** @return The current matchmaking state. */
	virtual EKronosMatchmakingState GetMatchmakingState() const;

	/** @return The result of the matchmaking. Only valid after the matchmaking has been completed. */
	virtual EKronosMatchmakingCompleteResult GetMatchmakingResult() const;

	/** @return The reason behind the matchmaking failure. Only valid after matchmaking has resulted in failure. */
	virtual EKronosMatchmakingFailureReason GetMatchmakingFailureReason() const;

	/** @return The current matchmaking policy object. */
	virtual UKronosMatchmakingPolicy* GetMatchmakingPolicy() const { return MatchmakingPolicy; }

	/** @return Get the search results of the latest matchmaking pass. */
	virtual TArray<FKronosSearchResult> GetMatchmakingSearchResults() const;

	/** @return The delegate fired when matchmaking is started. */
	FK2_OnStartKronosMatchmakingComplete& OnMatchmakingStarted() { return OnMatchmakingStartedEvent; }

	/** @return The delegate fired when matchmaking is complete. */
	FK2_OnKronosMatchmakingComplete& OnMatchmakingComplete() { return OnMatchmakingCompleteEvent; }

	/** @return The delegate fired when matchmaking is canceled. */
	FK2_OnCancelKronosMatchmakingComplete& OnMatchmakingCanceled() { return OnMatchmakingCanceledEvent; }
	
	/** @return The delegate fired when matchmaking state is changed. */
	FK2_OnKronosMatchmakingStateChanged& OnMatchmakingStateChanged() { return OnMatchmakingStateChangedEvent; }

	/** @return The delegate fired when either the matchmaking state or time changes. Helper delegate for UI elements. */
	FK2_OnKronosMatchmakingUpdated& OnMatchmakingUpdated() { return OnMatchmakingUpdatedEvent; }

	/** Dump current matchmaking settings to the console. */
	virtual void DumpMatchmakingSettings();

	/** Dump current matchmaking state to the console. */
	virtual void DumpMatchmakingState();

public:

	//~ Begin UObject interface
	virtual UWorld* GetWorld() const final; // Required by blueprints to have access to gameplay statics and other world context based nodes.
	//~ End UObject interface
};
