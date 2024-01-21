// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "CancelKronosMatchmakingProxy.generated.h"

/**
 * Proxy object that is responsible for handling cancel matchmaking requests.
 */
UCLASS()
class KRONOS_API UCancelKronosMatchmakingProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:

	/** Called when the matchmaking is canceled or if there was nothing to cancel. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnComplete;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

public:

	/**
	 * Cancels the currently active matchmaking policy if there is one.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", DisplayName = "Cancel Kronos Matchmaking", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UCancelKronosMatchmakingProxy* CancelKronosMatchmaking(UObject* WorldContextObject);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when matchmaking is canceled. */
	void OnCancelMatchmakingComplete();
};
