// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/CreateKronosSessionProxy.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingManager.h"

UCreateKronosSessionProxy* UCreateKronosSessionProxy::CreateKronosGameSession(UObject* WorldContextObject, const FKronosHostParams& HostParams, bool bBindGlobalEvents)
{
	UCreateKronosSessionProxy* Proxy = NewObject<UCreateKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_GameSession;
	Proxy->HostParams = HostParams;
	Proxy->bBindGlobalEvents = bBindGlobalEvents;
	return Proxy;
}

UCreateKronosSessionProxy* UCreateKronosSessionProxy::CreateKronosPartySession(UObject* WorldContextObject, const FKronosHostParams& HostParams, bool bBindGlobalEvents)
{
	UCreateKronosSessionProxy* Proxy = NewObject<UCreateKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_PartySession;
	Proxy->HostParams = HostParams;
	Proxy->bBindGlobalEvents = bBindGlobalEvents;
	return Proxy;
}

void UCreateKronosSessionProxy::Activate()
{
	UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
	MatchmakingManager->CreateMatchmakingPolicy(FOnCreateMatchmakingPolicyComplete::CreateUObject(this, &ThisClass::OnCreateKronosMatchmakingPolicyComplete), bBindGlobalEvents);
}

void UCreateKronosSessionProxy::OnCreateKronosMatchmakingPolicyComplete(UKronosMatchmakingPolicy* MatchmakingPolicy)
{
	if (MatchmakingPolicy)
	{
		MatchmakingPolicy->OnKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnKronosMatchmakingComplete);

		// Initialize matchmaking params from the host params.
		FKronosMatchmakingParams MatchmakingParams = FKronosMatchmakingParams(HostParams);
		uint8 MatchmakingFlags = 0;

		MatchmakingPolicy->StartMatchmaking(SessionName, MatchmakingParams, MatchmakingFlags, EKronosMatchmakingMode::CreateOnly);
		return;
	}

	OnKronosMatchmakingComplete(SessionName, EKronosMatchmakingCompleteResult::Failure);
}

void UCreateKronosSessionProxy::OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result)
{
	if (Result == EKronosMatchmakingCompleteResult::SessionCreated)
	{
		OnSuccess.Broadcast();
		return;
	}

	OnFailure.Broadcast();
	return;
}
