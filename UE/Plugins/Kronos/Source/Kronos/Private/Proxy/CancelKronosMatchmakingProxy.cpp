// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/CancelKronosMatchmakingProxy.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingManager.h"

UCancelKronosMatchmakingProxy* UCancelKronosMatchmakingProxy::CancelKronosMatchmaking(UObject* WorldContextObject)
{
	UCancelKronosMatchmakingProxy* Proxy = NewObject<UCancelKronosMatchmakingProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UCancelKronosMatchmakingProxy::Activate()
{
	UKronosMatchmakingPolicy* MatchmakingPolicy = UKronosMatchmakingManager::Get(WorldContextObject)->GetMatchmakingPolicy();
	if (MatchmakingPolicy)
	{
		if (MatchmakingPolicy->IsMatchmaking())
		{
			MatchmakingPolicy->OnCancelKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnCancelMatchmakingComplete);
			MatchmakingPolicy->CancelMatchmaking();
			return;
		}
	}

	OnCancelMatchmakingComplete();
}

void UCancelKronosMatchmakingProxy::OnCancelMatchmakingComplete()
{
	OnComplete.Broadcast();
}
