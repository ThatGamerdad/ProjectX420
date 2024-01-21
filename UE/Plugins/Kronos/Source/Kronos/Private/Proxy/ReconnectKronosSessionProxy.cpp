// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/ReconnectKronosSessionProxy.h"
#include "KronosUserManager.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingManager.h"
#include "KronosPartyManager.h"
#include "KronosConfig.h"
#include "Interfaces/OnlineFriendsInterface.h"

UReconnectKronosSessionProxy* UReconnectKronosSessionProxy::ReconnectKronosPartySession(UObject* WorldContextObject)
{
	UReconnectKronosSessionProxy* Proxy = NewObject<UReconnectKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	Proxy->LastPartyInfo = PartyManager->GetLastPartyInfo();

	return Proxy;
}

void UReconnectKronosSessionProxy::Activate()
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
	if (PartyManager->IsInParty())
	{
		UE_LOG(LogKronos, Error, TEXT("Reconnect party failed. Player already in a party."));
		OnCreateKronosMatchmakingPolicyComplete(nullptr);
		return;
	}

	if (!LastPartyInfo.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("Reconnect party failed. Last party information is invalid or none."));
		OnCreateKronosMatchmakingPolicyComplete(nullptr);
		return;
	}

	UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(WorldContextObject);
	MatchmakingManager->CreateMatchmakingPolicy(FOnCreateMatchmakingPolicyComplete::CreateUObject(this, &ThisClass::OnCreateKronosMatchmakingPolicyComplete), false);
	return;
}

void UReconnectKronosSessionProxy::OnCreateKronosMatchmakingPolicyComplete(UKronosMatchmakingPolicy* MatchmakingPolicy)
{
	if (MatchmakingPolicy)
	{
		if (LastPartyInfo.LastPartyRole == EKronosPartyRole::PartyHost)
		{
			// Use the previous party session's settings when re-creating the session.
			FKronosHostParams HostParams = FKronosHostParams();
			HostParams.SessionSettingsOverride = LastPartyInfo.LastPartySettings;
			HostParams.SessionSettingsOverride->Set(SETTING_RECONNECTID, LastPartyInfo.GetReconnectId(), EOnlineDataAdvertisementType::ViaOnlineService);

			// Make matchmaking params from host params.
			FKronosMatchmakingParams MatchmakingParams = FKronosMatchmakingParams(HostParams);

			MatchmakingPolicy->OnKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnKronosMatchmakingComplete);
			MatchmakingPolicy->StartMatchmaking(NAME_PartySession, MatchmakingParams, static_cast<uint8>(EKronosMatchmakingFlags::None), EKronosMatchmakingMode::CreateOnly);
			return;
		}

		if (LastPartyInfo.LastPartyRole == EKronosPartyRole::PartyClient)
		{
			FKronosMatchmakingParams MatchmakingParams = FKronosMatchmakingParams();
			MatchmakingParams.bIsLanQuery = LastPartyInfo.LastPartySettings.bIsLANMatch;
			MatchmakingParams.bSearchPresence = LastPartyInfo.LastPartySettings.bUsesPresence;
			MatchmakingParams.ExtraQuerySettings.Add(FKronosQuerySetting(SETTING_RECONNECTID, LastPartyInfo.GetReconnectId(), EOnlineComparisonOp::Equals));

			// Make the party query params.
			MatchmakingParams.SpecificSessionQuery = MakeSessionQueryParamsForClient();

			// Set max search attempts from config.
			// EloSearchAttempts because that's the one used by the search pass.
			MatchmakingParams.MaxSearchAttempts = 1;
			MatchmakingParams.EloSearchAttempts = GetDefault<UKronosConfig>()->ClientReconnectPartyAttempts;

			uint8 MatchmakingFlags = 0;
			MatchmakingFlags |= static_cast<uint8>(EKronosMatchmakingFlags::NoHost);
			MatchmakingFlags |= static_cast<uint8>(EKronosMatchmakingFlags::SkipReservation);
			MatchmakingFlags |= static_cast<uint8>(EKronosMatchmakingFlags::SkipEloChecks);

			// Delay the actual matchmaking so that the party leader can re-create the party.
			float StartDelay = GetDefault<UKronosConfig>()->ClientReconnectPartyDelay;

			MatchmakingPolicy->OnKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnKronosMatchmakingComplete);
			MatchmakingPolicy->StartMatchmaking(NAME_PartySession, MatchmakingParams, MatchmakingFlags, EKronosMatchmakingMode::Default, StartDelay);
			return;
		}
	}

	OnKronosMatchmakingComplete(NAME_PartySession, EKronosMatchmakingCompleteResult::Failure);
}

FKronosSpecificSessionQuery UReconnectKronosSessionProxy::MakeSessionQueryParamsForClient()
{
	/*
	 * There is an issue with Steam Online Subsystem FindFriendSession().
	 * I cannot get it to find the friend's session. It only works after a Steam invite has been received (doesn't have to accept it).
	 * Pretty sure an important API function isn't getting called somewhere in the OSS.
	 * I tried reading friends list and querying user presence data before using FindFriendSession, but these didn't make a difference at all.
	 * Steamworks SDK doesn't mention any required API calls either so I'm out of ideas.
	 *
	 * EOS doesn't support finding friend session in the first place.
	 * It supports FindSessionById instead but we can't use it because we are recreating the party session, so session id will be different.
	 *
	 * For now we'll rely on the base FindSession function.
	 * Will definitely need to sort this issue out in the future though.
	 */

	// Query via: friend id.
	if (GetDefault<UKronosConfig>()->bFindFriendSessionSupported)
	{
		UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
		if (UserManager->IsFriend(*LastPartyInfo.LastPartyHostPlayerId.GetUniqueNetId(), EFriendsLists::ToString(EFriendsLists::Default)))
		{
			FKronosSpecificSessionQuery QueryViaFriendId;
			QueryViaFriendId.Type = EKronosSpecificSessionQueryType::FriendId;
			QueryViaFriendId.UniqueId = LastPartyInfo.LastPartyHostPlayerId;
			return QueryViaFriendId;
		}
	}

	// Query via: session owner id.
	FKronosSpecificSessionQuery QueryViaOwnerId;
	QueryViaOwnerId.Type = EKronosSpecificSessionQueryType::SessionOwnerId;
	QueryViaOwnerId.UniqueId = LastPartyInfo.LastPartyHostPlayerId;
	return QueryViaOwnerId;
}

void UReconnectKronosSessionProxy::OnKronosMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result)
{
	if (Result >= EKronosMatchmakingCompleteResult::Success)
	{
		OnSuccess.Broadcast();
		return;
	}

	OnFailure.Broadcast();
	return;
}
