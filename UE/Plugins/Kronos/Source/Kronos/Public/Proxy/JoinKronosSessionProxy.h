// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "KronosTypes.h"
#include "JoinKronosSessionProxy.generated.h"

/**
 * Proxy object that is responsible for handling join only matchmaking requests.
 */
UCLASS()
class KRONOS_API UJoinKronosSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:

	/** Called when we joined the session successfully. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	/** Called when there was an error while joining the session. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

	/** Name of the session acted upon. */
	FName SessionName;

	/** The desired session we want to join. */
	FKronosSearchResult SessionToJoin;

	/** Whether the matchmaking should skip reservation requesting before joining the session. */
	bool bSkipReservation;

	/** Whether to bind global matchmaking events in the matchmaking manager. */
	bool bBindGlobalEvents;

public:

	/**
	 * Join the given game session using the online subsystem and connect to the host.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param SessionToJoin The desired session we want to join.
	 * @param bSkipReservation Whether we want to skip requesting a reservation before joining the session.
	 * @param bBindGlobalEvents Whether to bind global matchmaking events in the matchmaking manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Join Kronos Match", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UJoinKronosSessionProxy* JoinKronosGameSession(UObject* WorldContextObject, const FKronosSearchResult& SessionToJoin, bool bSkipReservation = false, bool bBindGlobalEvents = true);

	/**
	 * Join the given party session using the online subsystem and connect to the party.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param SessionToJoin The desired session we want to join.
	 * @param bSkipReservation Whether we want to skip requesting a reservation before joining the session.
	 * @param bBindGlobalEvents Whether to bind global matchmaking events in the matchmaking manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Join Kronos Party ", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UJoinKronosSessionProxy* JoinKronosPartySession(UObject* WorldContextObject, const FKronosSearchResult& SessionToJoin, bool bBindGlobalEvents = true);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when the matchmaking policy is created. */
	void OnCreateKronosMatchmakingPolicyComplete(class UKronosMatchmakingPolicy* MatchmakingPolicy);

	/** Called when the matchmaking is complete. */
	void OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result);
};
