// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KronosTypes.h"
#include "KronosMatchmakingStateWidget.generated.h"

/**
 * Widget for displaying the current matchmaking state.
 */
UCLASS()
class KRONOS_API UKronosMatchmakingStateWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Called when the matchmaking is started. */
	UFUNCTION()
	virtual void OnMatchmakingStarted();

	/** Called when the matchmaking is canceled. */
	UFUNCTION()
	virtual void OnMatchmakingCanceled();

	/** Called when the matchmaking is complete. */
	UFUNCTION()
	virtual void OnMatchmakingComplete(const FName SessionName, const EKronosMatchmakingCompleteResult Result);

	/** Called when the matchmaking state or time changes. */
	UFUNCTION()
	virtual void OnMatchmakingUpdated(const EKronosMatchmakingState MatchmakingState, const int32 MatchmakingTime);

protected:

	/** Event when the matchmaking is started. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Matchmaking Started")
	void K2_OnMatchmakingStarted();

	/** Event when the matchmaking is canceled. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Matchmaking Canceled")
	void K2_OnMatchmakingCanceled();

	/** Event when the matchmaking is complete. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Matchmaking Complete")
	void K2_OnMatchmakingComplete(const EKronosMatchmakingCompleteResult Result);

	/** Event when the matchmaking state or time changes. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Matchmaking Updated")
	void K2_OnMatchmakingUpdated(const EKronosMatchmakingState MatchmakingState, const int32 MatchmakingTime);

public:

	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface
};
