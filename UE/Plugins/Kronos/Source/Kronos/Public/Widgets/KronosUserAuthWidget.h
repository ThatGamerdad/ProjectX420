// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KronosUserAuthWidget.generated.h"

enum class EKronosUserAuthState : uint8;
enum class EKronosUserAuthCompleteResult : uint8;

/**
 * Widget that represents user authentication state.
 */
UCLASS(Abstract)
class KRONOS_API UKronosUserAuthWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Called when user authentication is started. */
	UFUNCTION()
	virtual void OnUserAuthStarted(bool bIsInitialAuth);

	/** Called when user authentication state is changed. */
	UFUNCTION()
	virtual void OnUserAuthStateChanged(EKronosUserAuthState NewState, EKronosUserAuthState PrevState, bool bIsInitialAuth);

	/** Called when user authentication is complete. */
	UFUNCTION()
	virtual void OnUserAuthComplete(EKronosUserAuthCompleteResult Result, bool bWasInitialAuth, const FText& ErrorText);

protected:

	/**
	 * Event when user authentication is started.
	 * 
	 * @param bIsInitialAuth Whether it's the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On User Auth Started")
	void K2_OnUserAuthStarted(bool bIsInitialAuth);

	/**
	 * Event when user authentication state is changed.
	 *
	 * @param NewState New authentication state.
	 * @param PrevState Previous authentication state.
	 * @param bIsInitialAuth Whether it's the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On User Auth State Changed")
	void K2_OnUserAuthStateChanged(EKronosUserAuthState NewState, EKronosUserAuthState PrevState, bool bIsInitialAuth);

	/**
	 * Event when user authentication is complete.
	 *
	 * @param Result Result of authentication. Anything other than Success is a form of failure and user should be prompted to restart authentication.
	 * @param bWasInitialAuth Whether this was the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
	 * @param ErrorText Detailed description of the error. Can be displayed to the user.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On User Auth Complete")
	void K2_OnUserAuthComplete(EKronosUserAuthCompleteResult Result, bool bWasInitialAuth, const FText& ErrorText);

public:

	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface
};
