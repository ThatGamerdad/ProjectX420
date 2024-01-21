// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/FindKronosSessionsProxy.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingManager.h"

UFindKronosSessionsProxy* UFindKronosSessionsProxy::FindKronosGameSessions(UObject* WorldContextObject, const FKronosSearchParams& SearchParams, bool bBindGlobalEvents)
{
	UFindKronosSessionsProxy* Proxy = NewObject<UFindKronosSessionsProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_GameSession;
	Proxy->SearchParams = SearchParams;
	Proxy->bBindGlobalEvents = bBindGlobalEvents;
	return Proxy;
}

UFindKronosSessionsProxy* UFindKronosSessionsProxy::FindKronosPartySessions(UObject* WorldContextObject, const FKronosSearchParams& SearchParams, bool bBindGlobalEvents)
{
	UFindKronosSessionsProxy* Proxy = NewObject<UFindKronosSessionsProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_PartySession;
	Proxy->SearchParams = SearchParams;
	Proxy->bBindGlobalEvents = bBindGlobalEvents;
	return Proxy;
}

void UFindKronosSessionsProxy::Activate()
{
	UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
	MatchmakingManager->CreateMatchmakingPolicy(FOnCreateMatchmakingPolicyComplete::CreateUObject(this, &ThisClass::OnCreateKronosMatchmakingPolicyComplete), bBindGlobalEvents);
}

void UFindKronosSessionsProxy::OnCreateKronosMatchmakingPolicyComplete(UKronosMatchmakingPolicy* MatchmakingPolicy)
{
	if (MatchmakingPolicy)
	{
		MatchmakingPolicy->OnKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnKronosMatchmakingComplete);

		// Initialize matchmaking params from search params.
		FKronosMatchmakingParams MatchmakingParams = FKronosMatchmakingParams(SearchParams);
		uint8 MatchmakingFlags = SearchParams.bSkipEloChecks ? static_cast<uint8>(EKronosMatchmakingFlags::SkipEloChecks) : 0;

		MatchmakingPolicy->StartMatchmaking(SessionName, MatchmakingParams, MatchmakingFlags, EKronosMatchmakingMode::SearchOnly);
		return;
	}

	OnKronosMatchmakingComplete(SessionName, EKronosMatchmakingCompleteResult::Failure);
}

void UFindKronosSessionsProxy::OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result)
{
	if (Result == EKronosMatchmakingCompleteResult::Success)
	{
		UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
		OnSuccess.Broadcast(MatchmakingManager->GetMatchmakingSearchResults());
		return;
	}

	TArray<FKronosSearchResult> EmptySearchResults = TArray<FKronosSearchResult>();
	OnFailure.Broadcast(EmptySearchResults);
	return;
}
