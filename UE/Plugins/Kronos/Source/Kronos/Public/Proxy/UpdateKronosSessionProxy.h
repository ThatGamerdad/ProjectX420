// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "KronosTypes.h"
#include "UpdateKronosSessionProxy.generated.h"

/**
 * Proxy object that is responsible for handling session update requests.
 */
UCLASS()
class KRONOS_API UUpdateKronosSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:

	/** Called when the session is updated. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	/** Called when there was an error while updating the session. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

	/** Name of the session acted upon. */
	FName SessionName;

	/** Settings to update the session with. */
	FKronosSessionSettings SessionSettings;

	/** List of session settings to update on the session. */
	TArray<FKronosSessionSetting> ExtraSessionSettings;

	/** Whether to submit the data to the backend or not. */
	bool bShouldRefreshOnlineData;

	/** Handle for session update request complete delegate. */
	FDelegateHandle OnUpdateSessionCompleteDelegateHandle;

public:

	/**
	 * Update the session settings of the match.
	 * Do NOT attempt to update a session while it is brand new (created / joined in the current frame).
	 * Otherwise you may experience weird issues. (E.g. the session cannot be found by others).
	 * 
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param SessionSettings Settings to update the session with.
	 * @param ExtraSessionSettings List of session settings to update on the session.
	 * @param bRefreshOnlineData Whether to submit the data to the backend or not.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Session", DisplayName = "Update Kronos Match", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AutoCreateRefTerm = "ExtraSessionSettings"))
	static UUpdateKronosSessionProxy* UpdateKronosGameSession(UObject* WorldContextObject, const FKronosSessionSettings& SessionSettings, const TArray<FKronosSessionSetting>& ExtraSessionSettings, const bool bRefreshOnlineData = true);

	/**
	 * Updates the session settings of the party.
	 * Do NOT attempt to update a session while it is brand new (created / joined in the current frame).
	 * Otherwise you may experience weird issues. (E.g. the session cannot be found by others).
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param SessionSettings Settings to update the session with.
	 * @param ExtraSessionSettings List of session settings to update on the session.
	 * @param bRefreshOnlineData Whether to submit the data to the backend or not.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Session", DisplayName = "Update Kronos Party", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AutoCreateRefTerm = "ExtraSessionSettings"))
	static UUpdateKronosSessionProxy* UpdateKronosPartySession(UObject* WorldContextObject, const FKronosSessionSettings& SessionSettings, const TArray<FKronosSessionSetting>& ExtraSessionSettings, const bool bRefreshOnlineData = true);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when the session is updated. */
	void OnUpdateSessionComplete(FName InSessionName, bool bWasSuccessful);
};
