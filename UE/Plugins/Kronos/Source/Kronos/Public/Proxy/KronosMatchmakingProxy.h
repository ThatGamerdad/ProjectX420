// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "KronosTypes.h"
#include "KronosMatchmakingProxy.generated.h"

/**
 * Delegate triggered when the matchmaking proxy is complete.
 *
 * @param Result The end result of the matchmaking. Only valid after the OnComplete pin.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosMatchmakingProxyComplete, const EKronosMatchmakingCompleteResult, Result);

/**
 * Proxy object that is responsible for handling matchmaking requests.
 */
UCLASS()
class KRONOS_API UKronosMatchmakingProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:

	/** Called when matchmaking is started. */
	UPROPERTY(BlueprintAssignable)
	FOnKronosMatchmakingProxyComplete OnStarted;

	/** Called when matchmaking is complete. */
	UPROPERTY(BlueprintAssignable)
	FOnKronosMatchmakingProxyComplete OnComplete;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

	/** Name of the session acted upon. */
	FName SessionName;

	/** Parameters to be used for matchmaking. */
	FKronosMatchmakingParams MatchmakingParams;

	/** Flags to be used for matchmaking. */
	uint8 MatchmakingFlags;

	/** Whether to bind global matchmaking events in the matchmaking manager. */
	bool bBindGlobalEvents;

public:

	/**
	 * Start matchmaking for a game session. Attempts to find and join the best available session.
	 * Can switch over to hosting role if no session is found.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param MatchmakingParams Parameters to be used for matchmaking.
	 * @param MatchmakingFlags Flags to be used for matchmaking.
	 * @param bBindGlobalEvents Whether to bind global matchmaking events in the matchmaking manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Start Kronos Matchmaking", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UKronosMatchmakingProxy* StartKronosGameSessionMatchmaking(UObject* WorldContextObject, const FKronosMatchmakingParams& MatchmakingParams, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/Kronos.EKronosMatchmakingFlags")) const int32 MatchmakingFlags = 0, bool bBindGlobalEvents = true);

	/**
	 * Start matchmaking for a party session. Attempts to find and join the best available session.
	 * Can switch over to hosting role if no session is found.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param MatchmakingParams Parameters to be used for matchmaking.
	 * @param MatchmakingFlags Flags to be used for matchmaking.
	 * @param bBindGlobalEvents Whether to bind global matchmaking events in the matchmaking manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Start Kronos Party Matchmaking", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UKronosMatchmakingProxy* StartKronosPartySessionMatchmaking(UObject* WorldContextObject, const FKronosMatchmakingParams& MatchmakingParams, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/Kronos.EKronosMatchmakingFlags")) const int32 MatchmakingFlags = 0x02, bool bBindGlobalEvents = true);
	
	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when the matchmaking policy is created. */
	void OnCreateKronosMatchmakingPolicyComplete(class UKronosMatchmakingPolicy* MatchmakingPolicy);

	/** Called when the matchmaking is started. */
	void OnKronosMatchmakingStarted();

	/** Called when the matchmaking is complete. */
	void OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result);
};
