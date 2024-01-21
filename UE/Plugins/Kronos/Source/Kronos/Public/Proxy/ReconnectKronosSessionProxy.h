// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "KronosTypes.h"
#include "ReconnectKronosSessionProxy.generated.h"

/**
 * Proxy object that is responsible for handling session reconnect requests.
 * Currently only party session reconnection is implemented.
 * Reconnecting a game session would be the same process though, we just need to figure out when to save previous session information. 
 */
UCLASS()
class KRONOS_API UReconnectKronosSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:

	/** Called when reconnecting the session was successful. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	/** Called when reconnecting the session failed. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

	/** Information about the last known party that we want to reconnect. */
	FKronosLastPartyInfo LastPartyInfo;

public:

	/**
	 * Attempt to recreate or rejoin the last known party based on the previous role of the player (party host, or party client).
	 * Last party data is updated automatically while in a party.
	 * In case the party is being recreated, all previous session settings will be re-applied and a special reconnect id will be set.
	 * Reconnecting party clients will match the hosting player id and the reconnect id to find and reconnect the party.
	 * 
	 * Make sure that both the party host and the party client calls this.
	 * Otherwise you'll end up in a situation where the party client is trying to find and connect to a party that doesn't exist, or doesn't have the reconnect id set.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Party", DisplayName = "Reconnect Kronos Party", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UReconnectKronosSessionProxy* ReconnectKronosPartySession(UObject* WorldContextObject);
	
	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when the matchmaking policy is created. */
	void OnCreateKronosMatchmakingPolicyComplete(class UKronosMatchmakingPolicy* MatchmakingPolicy);

	/** Create session query params to find the party with. Only party clients use this. */
	FKronosSpecificSessionQuery MakeSessionQueryParamsForClient();

	/** Called when the matchmaking is complete. */
	void OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result);
};
