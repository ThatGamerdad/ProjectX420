// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosConfig.h"
#include "KronosOnlineSession.h"
#include "KronosUserManager.h"
#include "KronosMatchmakingManager.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosMatchmakingSearchPass.h"
#include "KronosPartyManager.h"
#include "KronosReservationManager.h"
#include "Beacons/KronosPartyListener.h"
#include "Beacons/KronosPartyHost.h"
#include "Beacons/KronosPartyClient.h"
#include "Beacons/KronosPartyState.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "Beacons/KronosReservationListener.h"
#include "Beacons/KronosReservationHost.h"
#include "Beacons/KronosReservationClient.h"

UKronosConfig::UKronosConfig(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	OnlineSessionClass = UKronosOnlineSession::StaticClass();
	MatchmakingPolicyClass = UKronosMatchmakingPolicy::StaticClass();
	MatchmakingSearchPassClass = UKronosMatchmakingSearchPass::StaticClass();
	PartyListenerClass = AKronosPartyListener::StaticClass();
	PartyHostClass = AKronosPartyHost::StaticClass();
	PartyClientClass = AKronosPartyClient::StaticClass();
	PartyStateClass = AKronosPartyState::StaticClass();
	PartyPlayerStateClass = AKronosPartyPlayerState::StaticClass();
	ReservationListenerClass = AKronosReservationListener::StaticClass();
	ReservationHostClass = AKronosReservationHost::StaticClass();
	ReservationClientClass = AKronosReservationClient::StaticClass();
	UserManagerClass = UKronosUserManager::StaticClass();
	MatchmakingManagerClass = UKronosMatchmakingManager::StaticClass();
	PartyManagerClass = UKronosPartyManager::StaticClass();
	ReservationManagerClass = UKronosReservationManager::StaticClass();

	bAuthenticateUserAutomatically = true;
	MinTimePerAuthTask = 0.33f;
	EnterGameDelayAfterAuth = 0.0f;
	GameDefaultMapOverride = FSoftObjectPath();

	bFindFriendSessionSupported = true;
	bFindSessionByIdSupported = false;
	RestartMatchmakingPassDelay = 2.0f;
	RestartSearchPassDelay = 1.0f;
	SearchTimeout = 20.0f;

	ClientFollowPartyToSessionDelay = 4.0f;
	ClientFollowPartyAttempts = 5;
	ClientReconnectPartyDelay = 1.0f;
	ClientReconnectPartyAttempts = 5;

	ServerTravelToSessionDelay = 1.0f;
	ClientTravelToSessionDelay = 1.0f;

	ReservationTimeout = 60.0f;
}

const UKronosConfig* UKronosConfig::Get()
{
	return GetDefault<UKronosConfig>();
}
