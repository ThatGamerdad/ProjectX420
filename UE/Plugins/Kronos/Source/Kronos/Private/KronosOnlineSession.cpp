// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosOnlineSession.h"
#include "KronosUserManager.h"
#include "KronosMatchmakingManager.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosPartyManager.h"
#include "KronosReservationManager.h"
#include "Beacons/KronosPartyHost.h"
#include "Widgets/KronosUserAuthWidget.h"
#include "Kronos.h"
#include "KronosConfig.h"
#include "OnlineSubsystem.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "GameMapsSettings.h"

UKronosOnlineSession::UKronosOnlineSession(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnUpdateSessionCompleteDelegate = FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnUpdateSessionComplete);
		OnCleanupSessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCleanupSessionComplete);
		OnSessionUserInviteAcceptedDelegate = FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAccepted);
	}
}

UKronosOnlineSession* UKronosOnlineSession::Get(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		UGameInstance* GameInstance = World->GetGameInstance();
		UKronosOnlineSession* OnlineSession = GameInstance ? Cast<UKronosOnlineSession>(GameInstance->GetOnlineSession()) : nullptr;
		if (OnlineSession)
		{
			return OnlineSession;
		}

		UE_LOG(LogKronos, Error, TEXT("Failed to get KronosOnlineSession from GameInstance! Please make sure that your GameInstance class is set properly."));
		return nullptr;
	}

	UE_LOG(LogKronos, Error, TEXT("Failed to get KronosOnlineSession. Could not get World from the given WorldContextObject!"));
	return nullptr;
}

void UKronosOnlineSession::RegisterOnlineDelegates()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Initializing online managers..."));

	const UKronosConfig* KronosConfig = GetDefault<UKronosConfig>();

	// Create the managers.
	UObject* GameInstance = GetOuter();
	UserManager = NewObject<UKronosUserManager>(GameInstance, KronosConfig->UserManagerClass, TEXT("KronosUserManager"), RF_Transient);
	MatchmakingManager = NewObject<UKronosMatchmakingManager>(GameInstance, KronosConfig->MatchmakingManagerClass, TEXT("KronosMatchmakingManager"), RF_Transient);
	PartyManager = NewObject<UKronosPartyManager>(GameInstance, KronosConfig->PartyManagerClass, TEXT("KronosPartyManager"), RF_Transient);
	ReservationManager = NewObject<UKronosReservationManager>(GameInstance, KronosConfig->ReservationManagerClass, TEXT("KronosReservationManager"), RF_Transient);

	// Initialize all managers.
	UserManager->Initialize();
	MatchmakingManager->Initialize();
	PartyManager->Initialize();
	ReservationManager->Initialize();

	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Registering online delegates..."));

	// Bind the game mode initialized delegate.
	// We are going to use this to signal when the game default map is loaded and kick off auto login if required.
	GameModeInitializedDelegateHandle = FGameModeEvents::GameModeInitializedEvent.AddUObject(this, &ThisClass::OnGameModeInitialized);

	// Bind online subsystem delegates.
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		OnUpdateSessionCompleteDelegateHandle = SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(OnUpdateSessionCompleteDelegate);
		OnSessionUserInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
	}
}

void UKronosOnlineSession::ClearOnlineDelegates()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Clearing online delegates..."));

	// Deinitialize all managers.
	UserManager->Deinitialize();
	MatchmakingManager->Deinitialize();
	PartyManager->Deinitialize();
	ReservationManager->Deinitialize();

	FGameModeEvents::GameModeInitializedEvent.Remove(GameModeInitializedDelegateHandle);
	GameModeInitializedDelegateHandle.Reset();

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnUpdateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
	}
}

void UKronosOnlineSession::OnGameModeInitialized(AGameModeBase* GameMode)
{
	// Use the default game map as the authentication map if no overrides are given.
	FString DefaultGameMap = UGameMapsSettings::GetGameDefaultMap();

	// Override the authentication map if specified.
	const UKronosConfig* KronosConfig = GetDefault<UKronosConfig>();
	if (KronosConfig->GameDefaultMapOverride.IsValid())
	{
		DefaultGameMap = KronosConfig->GameDefaultMapOverride.GetLongPackageName();
	}

	UWorld* World = GetWorld();
	FString CurrentMap = World->RemovePIEPrefix(World->URL.Map);
	if (CurrentMap == DefaultGameMap)
	{
		// Notification that the game default map was loaded.
		// Delayed by one frame so that the local player controller gets created.
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::OnGameDefaultMapLoaded);
	}
}

void UKronosOnlineSession::OnGameDefaultMapLoaded()
{
	// Give blueprints a chance to implement custom logic.
	K2_OnGameDefaultMapLoaded();

	// Authenticate user automatically. No additional checks here because the user should be authenticated every time we enter the default map (main menu).
	// If the user was NOT authenticated, we must auth to enter the game.
	// If the user was authenticated before, we must auth to ensure that his login status is still valid.
	const UKronosConfig* KronosConfig = GetDefault<UKronosConfig>();
	if (KronosConfig->bAuthenticateUserAutomatically)
	{
		UserManager->AuthenticateUser();
	}
}

void UKronosOnlineSession::HandleUserAuthComplete(EKronosUserAuthCompleteResult Result, bool bWasInitialAuth, const FText& ErrorText)
{
	if (Result == EKronosUserAuthCompleteResult::Success)
	{
		// Delay entering the game if configured.
		// This is useful if you want to do some UI animation (e.g. screen fade).
		const UKronosConfig* KronosConfig = GetDefault<UKronosConfig>();
		if (KronosConfig->EnterGameDelayAfterAuth > 0.0f)
		{
			FTimerDelegate EnterGameDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::OnEnterGame, bWasInitialAuth);
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_EnterGame, EnterGameDelegate, KronosConfig->EnterGameDelayAfterAuth, false);
		}
		else
		{
			OnEnterGame(bWasInitialAuth);
		}
	}
}

void UKronosOnlineSession::OnEnterGame(bool bIsInitialLogin)
{
	// Remove the auth widget when entering the game.
	if (UserManager->AuthWidget)
	{
		UserManager->AuthWidget->RemoveFromParent();
		UserManager->AuthWidget = nullptr;
	}

	K2_OnEnterGame(bIsInitialLogin);
}

void UKronosOnlineSession::HandleMatchmakingComplete(const FName InSessionName, const EKronosMatchmakingCompleteResult Result)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Handling matchmaking complete event..."));

	switch (Result)
	{
	case EKronosMatchmakingCompleteResult::Failure:
		KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: No further actions required."));
		break;
	case EKronosMatchmakingCompleteResult::NoResults:
		KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: No further actions required."));
		break;
	case EKronosMatchmakingCompleteResult::Success:
		KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: No further actions required."));
		break;
	case EKronosMatchmakingCompleteResult::SessionCreated:
		HandleCreatingSession(InSessionName);
		break;
	case EKronosMatchmakingCompleteResult::SessionJoined:
		HandleJoiningSession(InSessionName);
		break;
	default:
		UE_LOG(LogKronos, Error, TEXT("Unhandled matchmaking result."));
		break;
	}
}

void UKronosOnlineSession::HandleCreatingSession(const FName InSessionName)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// Tell the session that we are going to host it. I'm not sure if I am supposed to set this myself, but the
			// online subsystem doesn't seem to change this value automatically.
			FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(InSessionName);
			NamedSession->bHosting = true;

			// Handle party session creation.
			if (InSessionName == NAME_PartySession)
			{
				const int32 PartySize = NamedSession->SessionSettings.NumPublicConnections;
				PartyManager->InitPartyBeaconHost(PartySize);
			}

			// Handle game session creation.
			else
			{
				if (PartyManager->IsPartyLeader())
				{
					// We have a party, connect them to the session.
					ConnectPartyToGameSession();
				}

				else
				{
					// We are not in a party, we can travel to the session right away.
					ServerTravelToGameSession();
				}
			}
		}
	}
}

void UKronosOnlineSession::HandleJoiningSession(const FName InSessionName)
{
	// Handle joining party session.
	if (InSessionName == NAME_PartySession)
	{
		PartyManager->InitPartyBeaconClient();
		return;
	}

	// Handle joining game session.
	else
	{
		if (PartyManager->IsPartyLeader())
		{
			// We have a party, connect them to the session.
			ConnectPartyToGameSession();
			return;
		}

		else
		{
			// We are not in a party, we can travel to the session right away.
			ClientTravelToGameSession();
			return;
		}
	}
}

bool UKronosOnlineSession::ConnectPartyToGameSession()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Connecting party to game session..."));

	if (!PartyManager->IsPartyLeader())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Only the party leader can connect party to game session."));
		return false;
	}

	return PartyManager->GetHostBeacon()->ProcessConnectPartyToGameSession();
}

bool UKronosOnlineSession::FollowPartyToGameSession(const FKronosFollowPartyParams& FollowPartyParams)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Following party to game session (Query type: %s)..."), LexToString(FollowPartyParams.SpecificSessionQuery.Type));

	if (!PartyManager->IsInParty() || PartyManager->IsPartyLeader())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosOnlineSession: Failed to follow party to game session. Player is not in a party, or is a party leader."));

		OnFollowPartyToSessionFailure();
		return false;
	}

	// Leave the party before matchmaking (even if matchmaking won't start due to an error or invalid params).
	PartyManager->LeavePartyInternal();

	if (!FollowPartyParams.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to follow party to game session. The given FKronosFollowPartyParams are invalid."));

		OnFollowPartyToSessionFailure();
		return false;
	}

	OnFollowPartyToSessionStarted();

	FOnCreateMatchmakingPolicyComplete CompletionDelegate = FOnCreateMatchmakingPolicyComplete::CreateLambda([this, FollowPartyParams](UKronosMatchmakingPolicy* MatchmakingPolicy)
	{
		if (MatchmakingPolicy)
		{
			MatchmakingPolicy->OnKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnFollowPartyToSessionComplete);
			MatchmakingPolicy->OnCancelKronosMatchmakingComplete().AddUObject(this, &ThisClass::OnFollowPartyToSessionFailure);

			// Initialize matchmaking params from the follow party params.
			FKronosMatchmakingParams MatchmakingParams = FKronosMatchmakingParams(FollowPartyParams);
			
			// Set max search attempts from config.
			// EloSearchAttempts because that's the one used by the search pass.
			MatchmakingParams.MaxSearchAttempts = 1;
			MatchmakingParams.EloSearchAttempts = GetDefault<UKronosConfig>()->ClientFollowPartyAttempts;

			uint8 MatchmakingFlags = 0;
			MatchmakingFlags |= static_cast<uint8>(EKronosMatchmakingFlags::NoHost);
			MatchmakingFlags |= static_cast<uint8>(EKronosMatchmakingFlags::SkipReservation);
			MatchmakingFlags |= static_cast<uint8>(EKronosMatchmakingFlags::SkipEloChecks);

			// Delay the actual matchmaking to give a bit of head start to the party leader.
			// This is needed for the online subsystem to change the currently advertised session (party -> game session).
			// Also if the party leader is hosting the match for the party, he needs to load the map first before accepting connections.

			float StartDelay = GetDefault<UKronosConfig>()->ClientFollowPartyToSessionDelay; // NOTE: FollowPartyParams.bPartyLeaderCreatingSession is currently not used.

			MatchmakingPolicy->StartMatchmaking(NAME_GameSession, MatchmakingParams, MatchmakingFlags, EKronosMatchmakingMode::Default, StartDelay);
			return;
		}
	});

	MatchmakingManager->CreateMatchmakingPolicy(CompletionDelegate, false);
	return true;
}

void UKronosOnlineSession::OnFollowPartyToSessionStarted()
{
	K2_OnFollowPartyToSessionStarted();
	PartyManager->OnFollowingPartyToSession().Broadcast();
}

void UKronosOnlineSession::OnFollowPartyToSessionComplete(const FName SessionName, const EKronosMatchmakingCompleteResult Result)
{
	// NOTE: This is going to be reworked because it doesn't make much sense for blueprints to get the matchmaking result.
	if (Result == EKronosMatchmakingCompleteResult::SessionJoined)
	{
		K2_OnFollowPartyToSessionComplete(SessionName, Result);
		return;
	}

	OnFollowPartyToSessionFailure();
}

void UKronosOnlineSession::OnFollowPartyToSessionFailure()
{
	K2_OnFollowPartyToSessionFailure();
	PartyManager->OnFollowPartyFailure().Broadcast();
}

void UKronosOnlineSession::ServerTravelToGameSession()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Attempting server travel to game session..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(NAME_GameSession);
			if (NamedSession)
			{
				if (!NamedSession->bHosting)
				{
					UE_LOG(LogKronos, Error, TEXT("We are not supposed to host the session -- NamedSession->bHosting = false"));
					return;
				}

				FString LevelName;
				if (NamedSession->SessionSettings.Get(SETTING_STARTINGLEVEL, LevelName))
				{
					// Since party beacons are going to be destroyed when changing maps anyways, we leave the party instead.
					// This way we are not going to get any network errors.
					if (PartyManager->IsInParty())
					{
						PartyManager->LeavePartyInternal();
					}

					// Create the travel URL and travel to the session after the delay.
					FString TravelURL = FString::Printf(TEXT("%s?listen?MaxPlayers=%d"), *LevelName, NamedSession->SessionSettings.NumPublicConnections);
					FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this, TravelURL]()
					{
						APlayerController* PrimaryPlayer = GetWorld()->GetFirstPlayerController();
						PrimaryPlayer->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
					});

					GetWorld()->GetTimerManager().SetTimer(TimerHandle_TravelToSession, TimerDelegate, GetDefault<UKronosConfig>()->ServerTravelToSessionDelay, false);
					return;
				}

				UE_LOG(LogKronos, Error, TEXT("Failed to get level name to open listen server on. Make sure that SETTING_STARTINGLEVEL is set when creating the session."));
				return;
			}

			UE_LOG(LogKronos, Error, TEXT("No GameSession exists."));
			return;
		}
	}

	UE_LOG(LogKronos, Error, TEXT("Session Interface invalid."));
	return;
}

void UKronosOnlineSession::ClientTravelToGameSession()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("Attempting client travel to game session..."));

	FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([this]()
	{
		// Resolve the connection with the session and travel to it.
		GetWorld()->GetGameInstance()->ClientTravelToSession(0, NAME_GameSession);
	});

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_TravelToSession, TimerDelegate, GetDefault<UKronosConfig>()->ClientTravelToSessionDelay, false);
}

void UKronosOnlineSession::HandleDisconnect(UWorld* World, UNetDriver* NetDriver)
{
	// Make sure that the disconnect was called for our world.
	if (World && World == GetWorld())
	{
		// Clean up our game session before returning to the main menu.
		if (!bHandlingCleanup)
		{
			CleanupSession(World, NetDriver);
			return;
		}
	}

	else
	{
		// It looks like the disconnect was not called for our world, so we'll just ignore it.
		// I'm not sure if this is even possible though.
	}
}

void UKronosOnlineSession::CleanupSession(UWorld* World, UNetDriver* NetDriver)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Cleaning up session..."));

	// Clean up our session before returning to the main menu. Called during HandleDisconnect() when a GameNetDriver is closed.
	// Keep in mind that this function may be called multiple times.
	// To avoid conflicts, we will begin handling the disconnect after a one frame delay.

	bHandlingCleanup = true;

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this, World]()
	{
		// Give blueprints a chance to implement custom logic.
		K2_OnCleanupSession();

		// Clean up our reservation host beacons.
		if (ReservationManager->IsReservationHost())
		{
			ReservationManager->DestroyReservationBeacons();
		}

		// Clean up the existing named session.
		// This is our local copy of the online session. It has to be destroyed by clients as well.
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem)
		{
			IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
			if (SessionInterface.IsValid())
			{
				EOnlineSessionState::Type SessionState = SessionInterface->GetSessionState(NAME_GameSession);
				if (SessionState != EOnlineSessionState::NoSession)
				{
					// Make sure that the session is not being destroyed already.
					if (SessionState == EOnlineSessionState::Destroying)
					{
						// In theory we should never hit this.
						// If the session is already being destroyed, a completion delegate must trigger somewhere so we'll wait for that.
						// However if that completion delegate doesn't call OnCleanupSessionComplete() then we are stuck.

						UE_LOG(LogKronos, Warning, TEXT("Session is already being destroyed. Waiting for completion delegate..."));
						return;
					}

					// Begin destroying the session. Completion delegate will trigger.
					else
					{
						SessionInterface->DestroySession(NAME_GameSession, OnCleanupSessionCompleteDelegate);
						return;
					}
				}
			}
		}

		// Finish cleanup.
		OnCleanupSessionComplete(NAME_GameSession, true);
	}));
}

void UKronosOnlineSession::OnCleanupSessionComplete(FName SessionName, bool bWasSuccessful)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: OnCleanupSessionComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	UWorld* World = GetWorld();
	UNetDriver* NetDriver = World ? World->GetNetDriver() : nullptr;

	// Let the engine handle the rest of the disconnect. This will transition us back to the main menu.
	GEngine->HandleDisconnect(World, NetDriver);

	bHandlingCleanup = false;
}

EOnlineSessionState::Type UKronosOnlineSession::GetSessionState(const FName SessionName) const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			return SessionInterface->GetSessionState(SessionName);
		}
	}

	return EOnlineSessionState::NoSession;
}

bool UKronosOnlineSession::GetSessionSettings(const FName SessionName, FKronosSessionSettings& OutSessionSettings)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FOnlineSessionSettings* SessionSettings = SessionInterface->GetSessionSettings(SessionName);
			if (SessionSettings)
			{
				OutSessionSettings = FKronosSessionSettings(*SessionSettings);
				return true;
			}

			UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to get session settings for '%s'. No session exists with the given name."), *SessionName.ToString());
			return false;
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to get session settings for '%s'. Session Interface invalid."), *SessionName.ToString());
	return false;
}

template<typename ValueType>
bool UKronosOnlineSession::GetSessionSetting(const FName SessionName, const FName Key, ValueType& OutValue)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(SessionName);
			if (NamedSession)
			{
				return NamedSession->SessionSettings.Get(Key, OutValue);
			}

			UE_LOG(LogKronos, Warning, TEXT("KronosOnlineSession: Failed to get '%s' session setting. No '%s' exists."), *Key.ToString(), *SessionName.ToString());
			return false;
		}
	}

	UE_LOG(LogKronos, Warning, TEXT("KronosOnlineSession: Failed to get '%s' session setting. Session interface is invalid."), *Key.ToString());
	return false;
}

// Explicit instantiation of supported types for the template above.
template KRONOS_API bool UKronosOnlineSession::GetSessionSetting(const FName SessionName, const FName Key, int32& OutValue);
template KRONOS_API bool UKronosOnlineSession::GetSessionSetting(const FName SessionName, const FName Key, float& OutValue);
template KRONOS_API bool UKronosOnlineSession::GetSessionSetting(const FName SessionName, const FName Key, uint64& OutValue);
template KRONOS_API bool UKronosOnlineSession::GetSessionSetting(const FName SessionName, const FName Key, double& OutValue);
template KRONOS_API bool UKronosOnlineSession::GetSessionSetting(const FName SessionName, const FName Key, FString& OutValue);
template KRONOS_API bool UKronosOnlineSession::GetSessionSetting(const FName SessionName, const FName Key, bool& OutValue);
template KRONOS_API bool UKronosOnlineSession::GetSessionSetting(const FName SessionName, const FName Key, TArray<uint8>& OutValue);

bool UKronosOnlineSession::UpdateSession(const FName SessionName, const FKronosSessionSettings& InSessionSettings, const bool bShouldRefreshOnlineData, const TArray<FKronosSessionSetting>& InExtraSessionSettings)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Updating %s..."), *SessionName.ToString());

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FOnlineSessionSettings* SessionSettings = SessionInterface->GetSessionSettings(SessionName);
			if (SessionSettings)
			{
				FOnlineSessionSettings UpdatedSessionSettings = FOnlineSessionSettings(*SessionSettings);

				// Updating session configuration.
				// NOTE: Some session settings cannot be updated after session creation (bIsLanMatch, bUsesPresence)

				UpdatedSessionSettings.NumPublicConnections = InSessionSettings.MaxNumPlayers;
				UpdatedSessionSettings.bShouldAdvertise = InSessionSettings.bShouldAdvertise;
				UpdatedSessionSettings.bAllowJoinInProgress = InSessionSettings.bAllowJoinInProgress;
				UpdatedSessionSettings.bAllowInvites = InSessionSettings.bAllowInvites;
				UpdatedSessionSettings.bAllowJoinViaPresence = InSessionSettings.bAllowJoinViaPresence;
				UpdatedSessionSettings.bUseLobbiesVoiceChatIfAvailable = InSessionSettings.bUseVoiceChatIfAvailable;

				UpdatedSessionSettings.Set(SETTING_SERVERNAME, InSessionSettings.ServerName, EOnlineDataAdvertisementType::ViaOnlineService);
				UpdatedSessionSettings.Set(SETTING_PLAYLIST, InSessionSettings.Playlist, EOnlineDataAdvertisementType::ViaOnlineService);
				UpdatedSessionSettings.Set(SETTING_MAPNAME, InSessionSettings.MapName, EOnlineDataAdvertisementType::ViaOnlineService);
				UpdatedSessionSettings.Set(SETTING_GAMEMODE, InSessionSettings.GameMode, EOnlineDataAdvertisementType::ViaOnlineService);
				UpdatedSessionSettings.Set(SETTING_SESSIONELO, InSessionSettings.Elo, EOnlineDataAdvertisementType::ViaOnlineService);
				UpdatedSessionSettings.Set(SETTING_SESSIONELO2, InSessionSettings.Elo, EOnlineDataAdvertisementType::ViaOnlineService);

				// Converting to int32 because the Steam Subsystem doesn't support bool queries.
				int32 bHidden = InSessionSettings.bHidden ? 1 : 0;
				UpdatedSessionSettings.Set(SETTING_HIDDEN, bHidden, EOnlineDataAdvertisementType::ViaOnlineService);

				// Update extra session settings.
				for (const FKronosSessionSetting& ExtraSetting : InExtraSessionSettings)
				{
					// Register the session setting the same way as UpdatedSessionSettings.Set(...) would
					{
						FOnlineSessionSetting* Setting = UpdatedSessionSettings.Settings.Find(ExtraSetting.Key);
						if (Setting)
						{
							Setting->Data = ExtraSetting.Data;
							Setting->AdvertisementType = ExtraSetting.AdvertisementType;
						}
						else
						{
							UpdatedSessionSettings.Settings.Add(ExtraSetting.Key, FOnlineSessionSetting(ExtraSetting.Data, ExtraSetting.AdvertisementType));
						}
					}
				}

				return SessionInterface->UpdateSession(SessionName, UpdatedSessionSettings, bShouldRefreshOnlineData);
			}

			UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to update %s - No session exists with the given name."), *SessionName.ToString());
			return false;
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to update %s - Session Interface invalid."), *SessionName.ToString());
	return false;
}

bool UKronosOnlineSession::RegisterPlayer(const FName SessionName, const FUniqueNetIdRepl& PlayerId, const bool bWasFromInvite)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			return SessionInterface->RegisterPlayer(SessionName, *PlayerId.GetUniqueNetId(), bWasFromInvite);
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to register player with session - Session Interface invalid."));
	return false;
}

bool UKronosOnlineSession::RegisterPlayers(const FName SessionName, const TArray<FUniqueNetIdRepl>& PlayerIds)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// Convert TArray<FUniqueNetIdRepl> to TArray<FUniqueNetIdRef>
			TArray<FUniqueNetIdRef> UniqueIds;
			for (const FUniqueNetIdRepl& PlayerId : PlayerIds)
			{
				if (PlayerId.IsValid())
				{
					const FUniqueNetIdPtr SharedPlayerId = PlayerId.GetUniqueNetId();
					UniqueIds.Add(SharedPlayerId.ToSharedRef());
				}
			}

			if (UniqueIds.Num() > 0)
			{
				return SessionInterface->RegisterPlayers(SessionName, UniqueIds, false);
			}

			else
			{
				UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to register players with session - PlayerIds were invalid."));
				return false;
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to register players with session - Session Interface invalid."));
	return false;
}

bool UKronosOnlineSession::UnregisterPlayer(const FName SessionName, const FUniqueNetIdRepl& PlayerId)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			return SessionInterface->UnregisterPlayer(SessionName, *PlayerId.GetUniqueNetId());
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to unregister player from session - Session Interface invalid."));
	return false;
}

bool UKronosOnlineSession::UnregisterPlayers(const FName SessionName, const TArray<FUniqueNetIdRepl>& PlayerIds)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// Convert TArray<FUniqueNetIdRepl> to TArray<FUniqueNetIdRef>
			TArray<FUniqueNetIdRef> UniqueIds;
			for (const FUniqueNetIdRepl& PlayerId : PlayerIds)
			{
				if (PlayerId.IsValid())
				{
					const FUniqueNetIdPtr SharedPlayerId = PlayerId.GetUniqueNetId();
					UniqueIds.Add(SharedPlayerId.ToSharedRef());
				}
			}

			if (UniqueIds.Num() > 0)
			{
				return SessionInterface->UnregisterPlayers(SessionName, UniqueIds);
			}

			else
			{
				UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to unregister players from session - PlayerIds were invalid."));
				return false;
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosOnlineSession: Failed to unregister players from session - Session Interface invalid."));
	return false;
}

bool UKronosOnlineSession::DestroySession(const FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			return SessionInterface->DestroySession(SessionName, CompletionDelegate);
		}
	}

	CompletionDelegate.ExecuteIfBound(SessionName, false);
	return false;
}

bool UKronosOnlineSession::BanPlayerFromSession(const FName SessionName, const FUniqueNetId& PlayerId)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Banning player %s from %s..."), *PlayerId.ToDebugString(), *SessionName.ToString());

	if (!PlayerId.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("PlayerId invalid."))
			return false;
	}

	if (IsPlayerBannedFromSession(SessionName, PlayerId))
	{
		UE_LOG(LogKronos, Warning, TEXT("Player '%s' already banned from session."), *PlayerId.ToDebugString())
			return false;
	}

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FOnlineSessionSettings* SessionSettings = SessionInterface->GetSessionSettings(SessionName);
			if (SessionSettings)
			{
				FOnlineSessionSettings UpdatedSessionSettings = FOnlineSessionSettings(*SessionSettings);

				FString BannedPlayers;
				UpdatedSessionSettings.Get(SETTING_BANNEDPLAYERS, BannedPlayers);

				// Add the player to the banned players string.
				// The expected format is "uniqueid1;uniqueid2;uniqueid3".
				{
					bool bAppendComma = !BannedPlayers.IsEmpty(); // Append comma before every unique id except the first
					BannedPlayers.Appendf(TEXT("%s%s"), bAppendComma ? TEXT(";") : TEXT(""), *PlayerId.ToString());
				}

				// Apply the changes.
				UpdatedSessionSettings.Set(SETTING_BANNEDPLAYERS, BannedPlayers, EOnlineDataAdvertisementType::ViaOnlineService);

				return SessionInterface->UpdateSession(SessionName, UpdatedSessionSettings, true);
			}

			UE_LOG(LogKronos, Error, TEXT("No '%s' exists."), *SessionName.ToString())
				return false;
		}
	}

	UE_LOG(LogKronos, Error, TEXT("Session Interface invalid."));
	return false;
}

bool UKronosOnlineSession::IsPlayerBannedFromSession(const FName SessionName, const FUniqueNetId& PlayerId)
{
	FString BannedPlayers;
	GetSessionSetting(SessionName, SETTING_BANNEDPLAYERS, BannedPlayers);

	if (!BannedPlayers.IsEmpty())
	{
		TArray<FString> BannedPlayersArray;
		if (BannedPlayers.ParseIntoArray(BannedPlayersArray, TEXT(";")) > 0)
		{
			if (BannedPlayersArray.Contains(PlayerId.ToString()))
			{
				return true;
			}
		}
	}

	return false;
}

void UKronosOnlineSession::StartOnlineSession(FName SessionName)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Starting %s..."), *SessionName.ToString());

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->StartSession(SessionName);
		}
	}
}

void UKronosOnlineSession::EndOnlineSession(FName SessionName)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: Ending %s..."), *SessionName.ToString());

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->EndSession(SessionName);
		}
	}
}

void UKronosOnlineSession::OnSessionUserInviteAccepted(const bool bWasSuccess, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: OnSessionUserInviteAccepted with result: %s"), bWasSuccess ? TEXT("Success") : TEXT("Failure"));

	if (bWasSuccess)
	{
		// Make Kronos search result from native search result.
		FKronosSearchResult SearchResult = FKronosSearchResult(InviteResult);
		bool bIsPartyInvite = SearchResult.GetSessionType() == NAME_PartySession;

		// Check if we are in a good state to accept the invite.
		if (CanAcceptSessionInvite(SearchResult, bIsPartyInvite))
		{
			// Handle invite received for a party.
			if (bIsPartyInvite)
			{
				OnPartySessionInviteAccepted(SearchResult);
				return;
			}

			// Handle invite received for a match.
			else
			{
				OnGameSessionInviteAccepted(SearchResult);
				return;
			}
		}
	}
}

void UKronosOnlineSession::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosOnlineSession: OnSessionUpdated with result: %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failure"));

	if (SessionName == NAME_PartySession)
	{
		OnUpdatePartyCompleteEvent.Broadcast(bWasSuccessful);
		return;
	}

	OnUpdatedMatchCompleteEvent.Broadcast(bWasSuccessful);
	return;
}

bool UKronosOnlineSession::CanAcceptSessionInvite_Implementation(const FKronosSearchResult& Session, bool bIsPartyInvite) const
{
	// Make sure that the Online Subsystem is valid.
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("Can't follow session invite. SessionInterface was invalid!"));
		return false;
	}

	// Make sure that the user is authenticated before accepting the invite.
	// This is going to break auto connecting session if the game was not running before accepting the invite (Steam '+connect_lobby' launch param).
	// I don't find this 'bug' a big issue to be honest since the game starts up properly. It's just that a new invite will be needed.
	// If you really want to get rid of this, you could cache the session received with the invite, and join it after user authentication is complete.
	if (!UserManager || !UserManager->IsAuthenticated() || UserManager->IsAuthenticatingUser())
	{
		UE_LOG(LogKronos, Error, TEXT("Can't follow session invite. Local user is not authenticated."));
		return false;
	}

	// Make sure that the player is not in a match before accepting an invite.
	// There is no real issue with accepting invites from a match, however only accepting invites from the main menu makes it much easier to manage the online state of the player.
	// There may be a bunch of unexpected edge cases otherwise.
	if (GetWorld()->GetNetMode() != NM_Standalone)
	{
		UE_LOG(LogKronos, Error, TEXT("Can't follow session invite. Local user is connected to a listen-server (most likely in a match)."));
		return false;
	}

	// Make sure that the session invite was not for a session that we are already a part of.
	FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(Session.GetSessionType());
	if (NamedSession)
	{
		const FUniqueNetId& SessionId = NamedSession->SessionInfo->GetSessionId();
		const FUniqueNetId& DestinationId = Session.GetSessionUniqueId();

		if (SessionId == DestinationId)
		{
			UE_LOG(LogKronos, Error, TEXT("Can't follow session invite. We are already in the session."));
			return false;
		}
	}

	return true;
}

void UKronosOnlineSession::OnGameSessionInviteAccepted(const FKronosSearchResult& Session)
{
	K2_OnGameSessionInviteAccepted(Session);

	MatchmakingManager->CreateMatchmakingPolicy(FOnCreateMatchmakingPolicyComplete::CreateLambda([Session](UKronosMatchmakingPolicy* MatchmakingPolicy)
	{
		MatchmakingPolicy->StartMatchmaking(Session.GetSessionType(), FKronosMatchmakingParams(), static_cast<uint8>(EKronosMatchmakingFlags::None), EKronosMatchmakingMode::JoinOnly, 0.0f, Session);
	}));
}

void UKronosOnlineSession::OnPartySessionInviteAccepted(const FKronosSearchResult& Session)
{
	K2_OnPartySessionInviteAccepted(Session);

	if (PartyManager->IsInParty())
	{
		// If we are in a party, leave the current party first.
		PartyManager->LeaveParty(FOnDestroySessionCompleteDelegate::CreateLambda([this, Session](FName SessionName, bool bWasSuccessful)
		{
			// Additional one-frame delay for safety measures.
			GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this, Session]()
			{
				// Join the new party.
				MatchmakingManager->CreateMatchmakingPolicy(FOnCreateMatchmakingPolicyComplete::CreateLambda([Session](UKronosMatchmakingPolicy* MatchmakingPolicy)
				{
					MatchmakingPolicy->StartMatchmaking(Session.GetSessionType(), FKronosMatchmakingParams(), static_cast<uint8>(EKronosMatchmakingFlags::None), EKronosMatchmakingMode::JoinOnly, 0.0f, Session);
				}));
			}));
		}));
	}

	else
	{
		MatchmakingManager->CreateMatchmakingPolicy(FOnCreateMatchmakingPolicyComplete::CreateLambda([Session](UKronosMatchmakingPolicy* MatchmakingPolicy)
		{
			MatchmakingPolicy->StartMatchmaking(Session.GetSessionType(), FKronosMatchmakingParams(), static_cast<uint8>(EKronosMatchmakingFlags::None), EKronosMatchmakingMode::JoinOnly, 0.0f, Session);
		}));
	}
}

FString UKronosOnlineSession::GetSessionDebugString(FName SessionName) const
{
	FString DebugString;

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return DebugString;
	}

	FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(SessionName);
	if (!NamedSession)
	{
		return DebugString;
	}

	DebugString.Appendf(TEXT("{grey}Session:\n"));
	DebugString.Appendf(TEXT("\tSessionName: {yellow}%s\n"), *NamedSession->SessionName.ToString());
	DebugString.Appendf(TEXT("\tHostingPlayerNum: {yellow}%d\n"), NamedSession->HostingPlayerNum);
	DebugString.Appendf(TEXT("\tSessionState: {yellow}%s\n"), EOnlineSessionState::ToString(NamedSession->SessionState));
	DebugString.Appendf(TEXT("\tRegisteredPlayers:\n"));
	if (NamedSession->RegisteredPlayers.Num())
	{
		for (int32 UserIdx = 0; UserIdx < NamedSession->RegisteredPlayers.Num(); UserIdx++)
		{
			DebugString.Appendf(TEXT("\t\t%d: {yellow}%s\n"), UserIdx, *NamedSession->RegisteredPlayers[UserIdx]->ToDebugString());
		}
	}
	else
	{
		DebugString.Appendf(TEXT("\t\t0 registered players\n"));
	}

	DebugString.Appendf(TEXT("\tOwningPlayerName: {yellow}%s\n"), *NamedSession->OwningUserName);
	DebugString.Appendf(TEXT("\tOwningPlayerId: {yellow}%s\n"), NamedSession->OwningUserId.IsValid() ? *NamedSession->OwningUserId->ToDebugString() : TEXT("INVALID"));
	DebugString.Appendf(TEXT("\tNumOpenPrivateConnections: {yellow}%d\n"), NamedSession->NumOpenPrivateConnections);
	DebugString.Appendf(TEXT("\tNumOpenPublicConnections: {yellow}%d\n"), NamedSession->NumOpenPublicConnections);
	DebugString.Appendf(TEXT("\tSessionInfo: {yellow}%s\n"), NamedSession->SessionInfo.IsValid() ? *NamedSession->SessionInfo->ToDebugString() : TEXT("NULL"));

	DebugString.Appendf(TEXT("\tNumPublicConnections: {yellow}%d\n"), NamedSession->SessionSettings.NumPublicConnections);
	DebugString.Appendf(TEXT("\tNumPrivateConnections: {yellow}%d\n"), NamedSession->SessionSettings.NumPrivateConnections);
	DebugString.Appendf(TEXT("\tbShouldAdvertise: {yellow}%s\n"), NamedSession->SessionSettings.bShouldAdvertise ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbAllowJoinInProgress: {yellow}%s\n"), NamedSession->SessionSettings.bAllowJoinInProgress ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbIsLanMatch: {yellow}%s\n"), NamedSession->SessionSettings.bIsLANMatch ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbIsDedicated: {yellow}%s\n"), NamedSession->SessionSettings.bIsDedicated ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbUsesStats: {yellow}%s\n"), NamedSession->SessionSettings.bUsesStats ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbAllowInvites: {yellow}%s\n"), NamedSession->SessionSettings.bAllowInvites ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbUsesPresence: {yellow}%s\n"), NamedSession->SessionSettings.bUsesPresence ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbAllowJoinViaPresence: {yellow}%s\n"), NamedSession->SessionSettings.bAllowJoinViaPresence ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbAllowJoinViaPresenceFriendsOnly: {yellow}%s\n"), NamedSession->SessionSettings.bAllowJoinViaPresenceFriendsOnly ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbAntiCheatProtected: {yellow}%s\n"), NamedSession->SessionSettings.bAntiCheatProtected ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbUseLobbiesIfAvailable: {yellow}%s\n"), NamedSession->SessionSettings.bUseLobbiesIfAvailable ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tbUseLobbiesVoiceChatIfAvailable: {yellow}%s\n"), NamedSession->SessionSettings.bUseLobbiesVoiceChatIfAvailable ? TEXT("True") : TEXT("False"));
	DebugString.Appendf(TEXT("\tBuildUniqueId: {yellow}0x%08x\n"), NamedSession->SessionSettings.BuildUniqueId);
	DebugString.Appendf(TEXT("\tSettings:\n"));
	for (FSessionSettings::TConstIterator It(NamedSession->SessionSettings.Settings); It; ++It)
	{
		FName Key = It.Key();
		const FOnlineSessionSetting& Setting = It.Value();
		DebugString.Appendf(TEXT("\t\t%s = {yellow}%s\n"), *Key.ToString(), *Setting.ToString());
	}

	return DebugString;
}

UWorld* UKronosOnlineSession::GetWorld() const
{
	// We don't care about the CDO because it's not relevant to world checks.
	// If we don't return nullptr here then blueprints are going to complain.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	return GetOuter()->GetWorld();
}
