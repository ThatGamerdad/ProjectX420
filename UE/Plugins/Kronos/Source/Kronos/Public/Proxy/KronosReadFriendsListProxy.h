// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "KronosReadFriendsListProxy.generated.h"

/**
 * Delegate triggered when the friends list have been read from the Online Subsystem.
 *
 * @param Friends List of friends read from the Online Subsystem.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosReadFriendsListComplete, const TArray<FKronosOnlineFriend>&, Friends);

/**
 * Proxy object that is responsible for reading friends lists from the Online Subsystem.
 */
UCLASS()
class KRONOS_API UKronosReadFriendsListProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:

	/** Called when the list of friends have been read successfully. */
	UPROPERTY(BlueprintAssignable)
	FOnKronosReadFriendsListComplete OnSuccess;

	/** Called when there was an error reading the friends list. */
	UPROPERTY(BlueprintAssignable)
	FOnKronosReadFriendsListComplete OnFailure;

private:

	/** The world context object in which this call is taking place. */
	UObject* WorldContextObject;

	/** Whether to read in game friends only. */
	bool bInGamePlayersOnly;

public:

	/**
	 * Read the friends list of the local player using the Online Subsystem.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param bInGamePlayersOnly Return in-game friends only. If false, all online friends are returned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Friends", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True"))
	static UKronosReadFriendsListProxy* ReadKronosFriendsList(UObject* WorldContextObject, bool bInGamePlayersOnly = true);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:

	/** Called when the friends list have been read from the Online Subsystem. */
	void OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
};
