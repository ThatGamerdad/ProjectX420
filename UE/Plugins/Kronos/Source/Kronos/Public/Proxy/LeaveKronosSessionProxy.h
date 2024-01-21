// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "LeaveKronosSessionProxy.generated.h"

/**
 * Proxy object that is responsible for handling the process of leaving a session.
 */
UCLASS()
class KRONOS_API ULeaveKronosSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:

	/** The completion event for the leave request. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnComplete;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

	/** Name of the session acted upon. */
	FName SessionName;

public:

	/**
	 * Leave the current match. The session will be destroyed and the player will return to the main menu.
	 * The completion event is called immediately after the return to main menu was requested.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Game", DisplayName = "Leave Kronos Match", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static ULeaveKronosSessionProxy* LeaveKronosGameSession(UObject* WorldContextObject);

	/**
	 * Leave the current party. The session and party beacons will be destroyed.
	 * The completion event is called when the party session is destroyed.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Party", DisplayName = "Leave Kronos Party", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static ULeaveKronosSessionProxy* LeaveKronosPartySession(UObject* WorldContextObject);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

protected:

	/** Called when the session is destroyed. */
	void OnLeavePartyComplete(FName InSessionName, bool bWasSuccessful);
};
