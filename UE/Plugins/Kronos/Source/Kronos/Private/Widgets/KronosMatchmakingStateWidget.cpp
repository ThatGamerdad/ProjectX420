// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Widgets/KronosMatchmakingStateWidget.h"
#include "KronosMatchmakingManager.h"

void UKronosMatchmakingStateWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(this);
	MatchmakingManager->OnMatchmakingStarted().AddDynamic(this, &ThisClass::OnMatchmakingStarted);
	MatchmakingManager->OnMatchmakingCanceled().AddDynamic(this, &ThisClass::OnMatchmakingCanceled);
	MatchmakingManager->OnMatchmakingComplete().AddDynamic(this, &ThisClass::OnMatchmakingComplete);
	MatchmakingManager->OnMatchmakingUpdated().AddDynamic(this, &ThisClass::OnMatchmakingUpdated);
}

void UKronosMatchmakingStateWidget::OnMatchmakingStarted()
{
	K2_OnMatchmakingStarted();
}

void UKronosMatchmakingStateWidget::OnMatchmakingCanceled()
{
	K2_OnMatchmakingCanceled();
}

void UKronosMatchmakingStateWidget::OnMatchmakingComplete(const FName SessionName, const EKronosMatchmakingCompleteResult Result)
{
	K2_OnMatchmakingComplete(Result);
}

void UKronosMatchmakingStateWidget::OnMatchmakingUpdated(const EKronosMatchmakingState MatchmakingState, const int32 MatchmakingTime)
{
	K2_OnMatchmakingUpdated(MatchmakingState, MatchmakingTime);
}
