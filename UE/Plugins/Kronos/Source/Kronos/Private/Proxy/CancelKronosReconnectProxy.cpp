// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/CancelKronosReconnectProxy.h"
#include "KronosOnlineSession.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingManager.h"
#include "KronosPartyManager.h"

UCancelKronosReconnectProxy* UCancelKronosReconnectProxy::CancelReconnectKronosPartySession(UObject* WorldContextObject)
{
	UCancelKronosReconnectProxy* Proxy = NewObject<UCancelKronosReconnectProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UCancelKronosReconnectProxy::Activate()
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

void UCancelKronosReconnectProxy::OnCancelMatchmakingComplete()
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	if (PartyManager->GetClientBeacon() != nullptr)
	{
		PartyManager->LeaveParty(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnLeavePartyComplete));
		return;
	}

	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
	if (OnlineSession)
	{
		EOnlineSessionState::Type PartySessionState = OnlineSession->GetSessionState(NAME_PartySession);
		if (PartySessionState != EOnlineSessionState::NoSession && PartySessionState != EOnlineSessionState::Destroying)
		{
			PartyManager->LeaveParty(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnLeavePartyComplete));
			return;
		}
	}
}

void UCancelKronosReconnectProxy::OnLeavePartyComplete(FName SessionName, bool bWasSuccessful)
{
	OnComplete.Broadcast();
}
