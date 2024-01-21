// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "CancelKronosReconnectProxy.generated.h"

/**
 * Proxy object that is responsible for canceling session reconnect requests.
 * Currently only party session reconnection is implemented.
 */
UCLASS()
class KRONOS_API UCancelKronosReconnectProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:

	/** Called when the reconnect request was canceled or there was nothing to cancel. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnComplete;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

public:

	/**
	 * Cancel the reconnect party request.
	 * Please note that currently there is no real state machine that manages reconnects, so this call will simply stop matchmaking and
	 * destroy party beacons regardless of their state. Make sure to only call this while reconnection is in progress.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Cancel Kronos Reconnect Party", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UCancelKronosReconnectProxy* CancelReconnectKronosPartySession(UObject* WorldContextObject);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when matchmaking is canceled. */
	void OnCancelMatchmakingComplete();

	/** Called when player left the party. */
	void OnLeavePartyComplete(FName SessionName, bool bWasSuccessful);
};
