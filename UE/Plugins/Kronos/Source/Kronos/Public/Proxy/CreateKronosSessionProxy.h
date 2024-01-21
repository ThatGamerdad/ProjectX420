// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "KronosTypes.h"
#include "CreateKronosSessionProxy.generated.h"

/**
 * Proxy object that is responsible for handling create only matchmaking requests.
 */
UCLASS()
class KRONOS_API UCreateKronosSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:

	/** Called when the session is created. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	/** Called when there was an error during session creation. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

	/** Name of the session acted upon. */
	FName SessionName;

	/** Parameters to be used when creating the session. */
	FKronosHostParams HostParams;

	/** Whether to bind global matchmaking events in the matchmaking manager. */
	bool bBindGlobalEvents;

public:

	/**
	 * Create a new game session using the online subsystem and begin hosting a match.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param HostParams Parameters to be used when creating the session.
	 * @param bBindGlobalEvents Whether to bind global matchmaking events in the matchmaking manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Create Kronos Match", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UCreateKronosSessionProxy* CreateKronosGameSession(UObject* WorldContextObject, const FKronosHostParams& HostParams, bool bBindGlobalEvents = true);

	/**
	 * Create a new party session using the online subsystem and initialize a party host beacon.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param HostParams Parameters to be used when creating the session.
	 * @param bBindGlobalEvents Whether to bind global matchmaking events in the matchmaking manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Create Kronos Party", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UCreateKronosSessionProxy* CreateKronosPartySession(UObject* WorldContextObject, const FKronosHostParams& HostParams, bool bBindGlobalEvents = true);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when the matchmaking policy is created. */
	void OnCreateKronosMatchmakingPolicyComplete(class UKronosMatchmakingPolicy* MatchmakingPolicy);

	/** Called when the matchmaking is complete. */
	void OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result);
};
