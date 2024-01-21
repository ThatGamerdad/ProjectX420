// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosMatchmakingManager.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingSearchPass.h"
#include "KronosOnlineSession.h"
#include "KronosConfig.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UKronosMatchmakingManager* UKronosMatchmakingManager::Get(const UObject* WorldContextObject)
{
	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
	if (OnlineSession)
	{
		return OnlineSession->GetMatchmakingManager();
	}

	return nullptr;
}

void UKronosMatchmakingManager::CreateMatchmakingPolicy(const FOnCreateMatchmakingPolicyComplete& CompletionDelegate, bool bBindGlobalDelegates, bool bAutoHandleCompletion)
{
	// Check if there is an existing matchmaking policy.
	if (MatchmakingPolicy)
	{
		// If matchmaking is in progress, cancel it before creating a new policy.
		if (MatchmakingPolicy->IsMatchmaking())
		{
			MatchmakingPolicy->OnCancelKronosMatchmakingComplete().AddLambda([this, CompletionDelegate, bBindGlobalDelegates, bAutoHandleCompletion]()
			{
				// At this point the previous matchmaking has been canceled.
				// We will create a new matchmaking policy but only after a one frame delay! This delay is necessary because at this point the
				// cancel completion delegate is still executing. If we were to invalidate the policy now we would crash.

				FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this, CompletionDelegate, bBindGlobalDelegates, bAutoHandleCompletion]()
				{
					// At this point we are certain that the cancel completion delegate is done broadcasting. Attempt to create the policy again.
					CreateMatchmakingPolicy(CompletionDelegate, bBindGlobalDelegates, bAutoHandleCompletion);
					return;
				});

				// Begin waiting for one frame. The delegate above will execute after the delay.
				GetWorld()->GetTimerManager().SetTimerForNextTick(TimerDelegate);
				return;
			});

			// Begin canceling the matchmaking. The delegate above will execute once canceling is complete.
			MatchmakingPolicy->CancelMatchmaking();
			return;
		}

		// At this point we are certain that no matchmaking is in progress.
		// We can safely invalidate the previous matchmaking policy and begin creating a new one.

		MatchmakingPolicy->Invalidate();
		MatchmakingPolicy = nullptr;
	}

	// Create the new matchmaking policy and register it with the subsystem.
	UKronosMatchmakingPolicy* NewPolicy = NewObject<UKronosMatchmakingPolicy>(this, GetDefault<UKronosConfig>()->MatchmakingPolicyClass);
	RegisterMatchmakingPolicy(NewPolicy, bBindGlobalDelegates, bAutoHandleCompletion);

	CompletionDelegate.ExecuteIfBound(NewPolicy);
}

void UKronosMatchmakingManager::RegisterMatchmakingPolicy(UKronosMatchmakingPolicy* InPolicy, bool bBindGlobalDelegates, bool bAutoHandleCompletion)
{
	if (!InPolicy)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosMatchmakingManager: RegisterMatchmakingPolicy() was called with nullptr."))
		return;
	}

	// Register the new matchmaking policy.
	MatchmakingPolicy = InPolicy;

	// Bind the function that will handle matchmaking completion (e.g. begin hosting a match if matchmaking resulted in session creation).
	if (bAutoHandleCompletion)
	{
		UKronosOnlineSession* KronosOnlineSession = UKronosOnlineSession::Get(this);
		if (KronosOnlineSession)
		{
			MatchmakingPolicy->OnKronosMatchmakingComplete().AddUObject(KronosOnlineSession, &UKronosOnlineSession::HandleMatchmakingComplete);
		}
	}

	// Bind global matchmaking delegates.
	if (bBindGlobalDelegates)
	{
		MatchmakingPolicy->OnStartKronosMatchmakingComplete().AddLambda([this]() { OnMatchmakingStartedEvent.Broadcast(); });
		MatchmakingPolicy->OnKronosMatchmakingComplete().AddLambda([this](const FName SessionName, const EKronosMatchmakingCompleteResult Result) { OnMatchmakingCompleteEvent.Broadcast(SessionName, Result); });
		MatchmakingPolicy->OnCancelKronosMatchmakingComplete().AddLambda([this]() { OnMatchmakingCanceledEvent.Broadcast(); });
		MatchmakingPolicy->OnKronosMatchmakingStateChanged().AddLambda([this](const EKronosMatchmakingState OldState, const EKronosMatchmakingState NewState) { OnMatchmakingStateChangedEvent.Broadcast(OldState, NewState); });
		MatchmakingPolicy->OnKronosMatchmakingUpdated().AddLambda([this](const EKronosMatchmakingState MatchmakingState, const int32 MatchmakingTime) { OnMatchmakingUpdatedEvent.Broadcast(MatchmakingState, MatchmakingTime); });
	}
}

bool UKronosMatchmakingManager::IsMatchmaking() const
{
	return MatchmakingPolicy && MatchmakingPolicy->IsMatchmaking();
}

EKronosMatchmakingState UKronosMatchmakingManager::GetMatchmakingState() const
{
	if (MatchmakingPolicy)
	{
		return MatchmakingPolicy->GetMatchmakingState();
	}

	return EKronosMatchmakingState::NotStarted;
}

EKronosMatchmakingCompleteResult UKronosMatchmakingManager::GetMatchmakingResult() const
{
	if (MatchmakingPolicy)
	{
		return MatchmakingPolicy->GetMatchmakingResult();
	}

	return EKronosMatchmakingCompleteResult::Failure;
}

EKronosMatchmakingFailureReason UKronosMatchmakingManager::GetMatchmakingFailureReason() const
{
	if (MatchmakingPolicy)
	{
		return MatchmakingPolicy->GetFailureReason();
	}

	return EKronosMatchmakingFailureReason::Unknown;
}

TArray<FKronosSearchResult> UKronosMatchmakingManager::GetMatchmakingSearchResults() const
{
	if (MatchmakingPolicy)
	{
		UKronosMatchmakingSearchPass* SearchPass = MatchmakingPolicy->GetSearchPass();
		if (SearchPass)
		{
			return SearchPass->GetSearchResults();
		}
	}

	return TArray<FKronosSearchResult>();
}

void UKronosMatchmakingManager::DumpMatchmakingSettings()
{
	if (MatchmakingPolicy)
	{
		MatchmakingPolicy->DumpSettings();
		return;
	}

	UE_LOG(LogKronos, Log, TEXT("No matchmaking policy to dump settings for."));
}

void UKronosMatchmakingManager::DumpMatchmakingState()
{
	if (MatchmakingPolicy)
	{
		MatchmakingPolicy->DumpMatchmakingState();
		return;
	}

	UE_LOG(LogKronos, Log, TEXT("No matchmaking policy to dump state for."));
}

UWorld* UKronosMatchmakingManager::GetWorld() const
{
	// We don't care about the CDO because it's not relevant to world checks.
	// If we don't return nullptr here then blueprints are going to complain.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	return GetOuter()->GetWorld();
}
