// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Widgets/KronosUserAuthWidget.h"
#include "KronosUserManager.h"
#include "KronosTypes.h"

void UKronosUserAuthWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UKronosUserManager* UserManager = UKronosUserManager::Get(this);
	UserManager->OnKronosUserAuthStarted().AddDynamic(this, &ThisClass::OnUserAuthStarted);
	UserManager->OnKronosUserAuthStateChanged().AddDynamic(this, &ThisClass::OnUserAuthStateChanged);
	UserManager->OnKronosUserAuthComplete().AddDynamic(this, &ThisClass::OnUserAuthComplete);

	// Handle cases where user auth is already in progress when widget is added to the screen.
	EKronosUserAuthState CurrentAuthState = UserManager->GetCurrentAuthState();
	if (CurrentAuthState != EKronosUserAuthState::NotAuthenticating)
	{
		bool bIsInitialAuth = !UserManager->IsAuthenticated();
		OnUserAuthStarted(bIsInitialAuth);
		OnUserAuthStateChanged(CurrentAuthState, EKronosUserAuthState::NotAuthenticating, bIsInitialAuth);
	}
}

void UKronosUserAuthWidget::OnUserAuthStarted(bool bIsInitialAuth)
{
	K2_OnUserAuthStarted(bIsInitialAuth);
}

void UKronosUserAuthWidget::OnUserAuthStateChanged(EKronosUserAuthState NewState, EKronosUserAuthState PrevState, bool bIsInitialAuth)
{
	K2_OnUserAuthStateChanged(NewState, PrevState, bIsInitialAuth);
}

void UKronosUserAuthWidget::OnUserAuthComplete(EKronosUserAuthCompleteResult Result, bool bWasInitialAuth, const FText& ErrorText)
{
	K2_OnUserAuthComplete(Result, bWasInitialAuth, ErrorText);
}
