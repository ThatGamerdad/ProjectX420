// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/KronosMatchmakingProxy.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingManager.h"

UKronosMatchmakingProxy* UKronosMatchmakingProxy::StartKronosGameSessionMatchmaking(UObject* WorldContextObject, const FKronosMatchmakingParams& MatchmakingParams, const int32 MatchmakingFlags, bool bBindGlobalEvents)
{
	UKronosMatchmakingProxy* Proxy = NewObject<UKronosMatchmakingProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_GameSession;
	Proxy->MatchmakingParams = MatchmakingParams;
	Proxy->MatchmakingFlags = MatchmakingFlags;
	Proxy->bBindGlobalEvents = bBindGlobalEvents;
	return Proxy;
}

UKronosMatchmakingProxy* UKronosMatchmakingProxy::StartKronosPartySessionMatchmaking(UObject* WorldContextObject, const FKronosMatchmakingParams& MatchmakingParams, const int32 MatchmakingFlags, bool bBindGlobalEvents)
{
	UKronosMatchmakingProxy* Proxy = NewObject<UKronosMatchmakingProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_PartySession;
	Proxy->MatchmakingParams = MatchmakingParams;
	Proxy->MatchmakingFlags = MatchmakingFlags;
	Proxy->bBindGlobalEvents = bBindGlobalEvents;
	return Proxy;
}

void UKronosMatchmakingProxy::Activate()
{
	UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
	MatchmakingManager->CreateMatchmakingPolicy(FOnCreateMatchmakingPolicyComplete::CreateUObject(this, &ThisClass::OnCreateKronosMatchmakingPolicyComplete), bBindGlobalEvents);
}

void UKronosMatchmakingProxy::OnCreateKronosMatchmakingPolicyComplete(UKronosMatchmakingPolicy* MatchmakingPolicy)
{
	if (MatchmakingPolicy)
	{
		MatchmakingPolicy->OnStartKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnKronosMatchmakingStarted);
		MatchmakingPolicy->OnKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnKronosMatchmakingComplete);

		MatchmakingPolicy->StartMatchmaking(SessionName, MatchmakingParams, MatchmakingFlags);
		return;
	}

	OnKronosMatchmakingComplete(SessionName, EKronosMatchmakingCompleteResult::Failure);
}

void UKronosMatchmakingProxy::OnKronosMatchmakingStarted()
{
	// The OnStarted delegate is actually an FOnKronosMatchmakingProxyComplete delegate, so we must pass in a result.
	// This is because all delegates in a proxy must be the same. Otherwise delegate params won't show up in the BP node.

	EKronosMatchmakingCompleteResult InvalidResult = EKronosMatchmakingCompleteResult::NoResults;
	OnStarted.Broadcast(InvalidResult);
}

void UKronosMatchmakingProxy::OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result)
{
	OnComplete.Broadcast(Result);
}
