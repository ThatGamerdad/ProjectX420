// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "KronosTypes.h"
#include "FindKronosSessionsProxy.generated.h"

/**
 * Delegate triggered when the find sessions proxy is complete.
 *
 * @param Results Sessions found during the search.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindKronosSessionsProxyComplete, const TArray<FKronosSearchResult>&, Results);

/**
 * Proxy object that is responsible for handling search only matchmaking requests.
 */
UCLASS()
class KRONOS_API UFindKronosSessionsProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:

	/** Called when the search is complete. It is possible that no sessions were found. */
	UPROPERTY(BlueprintAssignable)
	FOnFindKronosSessionsProxyComplete OnSuccess;

	/** Called when there was an error during the search. */
	UPROPERTY(BlueprintAssignable)
	FOnFindKronosSessionsProxyComplete OnFailure;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

	/** Name of the session acted upon. */
	FName SessionName;

	/** Parameters to be used when searching for sessions. */
	FKronosSearchParams SearchParams;

	/** Whether to bind global matchmaking events in the matchmaking manager. */
	bool bBindGlobalEvents;

public:

	/**
	 * Begins searching for game sessions using the online subsystem.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param SearchParams Parameters to be used when searching for sessions.
	 * @param bBindGlobalEvents Whether to bind global matchmaking events in the matchmaking manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Find Kronos Matches", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UFindKronosSessionsProxy* FindKronosGameSessions(UObject* WorldContextObject, const FKronosSearchParams& SearchParams, bool bBindGlobalEvents = true);

	/**
	 * Begins searching for party sessions using the online subsystem.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param SearchParams Parameters to be used when searching for sessions.
	 * @param bBindGlobalEvents Whether to bind global matchmaking events in the matchmaking manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Find Kronos Parties", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UFindKronosSessionsProxy* FindKronosPartySessions(UObject* WorldContextObject, const FKronosSearchParams& SearchParams, bool bBindGlobalEvents = true);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when the matchmaking policy is created. */
	void OnCreateKronosMatchmakingPolicyComplete(class UKronosMatchmakingPolicy* MatchmakingPolicy);

	/** Called when the matchmaking is complete. */
	void OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result);
};
