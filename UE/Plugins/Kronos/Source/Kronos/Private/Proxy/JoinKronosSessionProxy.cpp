// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/JoinKronosSessionProxy.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingManager.h"

UJoinKronosSessionProxy* UJoinKronosSessionProxy::JoinKronosGameSession(UObject* WorldContextObject, const FKronosSearchResult& SessionToJoin, bool bSkipReservation, bool bBindGlobalEvents)
{
	UJoinKronosSessionProxy* Proxy = NewObject<UJoinKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_GameSession;
	Proxy->SessionToJoin = SessionToJoin;
	Proxy->bSkipReservation = bSkipReservation;
	Proxy->bBindGlobalEvents = bBindGlobalEvents;
	return Proxy;
}

UJoinKronosSessionProxy* UJoinKronosSessionProxy::JoinKronosPartySession(UObject* WorldContextObject, const FKronosSearchResult& SessionToJoin, bool bBindGlobalEvents)
{
	UJoinKronosSessionProxy* Proxy = NewObject<UJoinKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_PartySession;
	Proxy->SessionToJoin = SessionToJoin;
	Proxy->bSkipReservation = true;
	Proxy->bBindGlobalEvents = bBindGlobalEvents;
	return Proxy;
}

void UJoinKronosSessionProxy::Activate()
{
	UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
	MatchmakingManager->CreateMatchmakingPolicy(FOnCreateMatchmakingPolicyComplete::CreateUObject(this, &ThisClass::OnCreateKronosMatchmakingPolicyComplete), bBindGlobalEvents);
}

void UJoinKronosSessionProxy::OnCreateKronosMatchmakingPolicyComplete(UKronosMatchmakingPolicy* MatchmakingPolicy)
{
	if (MatchmakingPolicy)
	{
		MatchmakingPolicy->OnKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnKronosMatchmakingComplete);

		FKronosMatchmakingParams MatchmakingParams = FKronosMatchmakingParams(); // Matchmaking params doesn't matter in JoinOnly mode.
		uint8 MatchmakingFlags = bSkipReservation ? static_cast<uint8>(EKronosMatchmakingFlags::SkipReservation) : 0;

		MatchmakingPolicy->StartMatchmaking(SessionName, MatchmakingParams, MatchmakingFlags, EKronosMatchmakingMode::JoinOnly, 0.0f, SessionToJoin);
		return;
	}

	OnKronosMatchmakingComplete(SessionName, EKronosMatchmakingCompleteResult::Failure);
}

void UJoinKronosSessionProxy::OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result)
{
	if (Result == EKronosMatchmakingCompleteResult::SessionJoined)
	{
		OnSuccess.Broadcast();
		return;
	}

	OnFailure.Broadcast();
	return;
}
