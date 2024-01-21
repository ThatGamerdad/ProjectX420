// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosReservationManager.h"
#include "KronosPartyManager.h"
#include "KronosOnlineSession.h"
#include "Beacons/KronosReservationListener.h"
#include "Beacons/KronosReservationHost.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "KronosConfig.h"
#include "Kronos.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DebugCameraController.h"

UKronosReservationManager* UKronosReservationManager::Get(const UObject* WorldContextObject)
{
	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
	if (OnlineSession)
	{
		return OnlineSession->GetReservationManager();
	}

	return nullptr;
}

void UKronosReservationManager::Initialize()
{
	FGameModeEvents::OnGameModeInitializedEvent().AddUObject(this, &ThisClass::OnGameModeInitialized);
	FGameModeEvents::OnGameModePreLoginEvent().AddUObject(this, &ThisClass::OnGameModePreLogin);
	FGameModeEvents::OnGameModePostLoginEvent().AddUObject(this, &ThisClass::OnGameModePostLogin);
	FGameModeEvents::OnGameModeLogoutEvent().AddUObject(this, &ThisClass::OnGameModeLogout);
}

bool UKronosReservationManager::InitReservationBeaconHost(const int32 MaxReservations)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationManager: Creating reservation beacon host..."));

	ReservationBeaconListener = GetWorld()->SpawnActor<AKronosReservationListener>(GetDefault<UKronosConfig>()->ReservationListenerClass);
	if (ReservationBeaconListener)
	{
		if (ReservationBeaconListener->InitHost())
		{
			UE_LOG(LogKronos, Verbose, TEXT("Beacon host listener initialized."));

			ReservationBeaconHost = GetWorld()->SpawnActor<AKronosReservationHost>(GetDefault<UKronosConfig>()->ReservationHostClass);
			if (ReservationBeaconHost)
			{
				if (ReservationBeaconHost->InitHostBeacon(MaxReservations))
				{
					UE_LOG(LogKronos, Verbose, TEXT("Beacon host initialized."));

					ReservationBeaconListener->RegisterHost(ReservationBeaconHost);
					ReservationBeaconListener->PauseBeaconRequests(false);

					ReservationBeaconHost->OnInitialized();

					KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationManager: Registering host reservations..."));
					RegisterHostReservations();

					KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationManager: Reservation host beacon initialized."));
					return true;
				}
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosReservationManager: InitReservationBeaconHost failed."));
	return false;
}

bool UKronosReservationManager::ReconfigureMaxReservations(const int32 InMaxReservations)
{
	if (IsReservationHost())
	{
		return ReservationBeaconHost->ReconfigureMaxReservations(InMaxReservations);
	}

	return false;
}

void UKronosReservationManager::DestroyReservationBeacons()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationManager: Destroying reservation beacons..."));

	if (ReservationBeaconHost)
	{
		ReservationBeaconHost->Destroy();
		ReservationBeaconHost = nullptr;
	}

	if (ReservationBeaconListener)
	{
		ReservationBeaconListener->DestroyBeacon();
		ReservationBeaconListener = nullptr;
	}
}

bool UKronosReservationManager::SetHostReservation(const FKronosReservation& InReservation)
{
	TArray<FKronosReservation> NewHostReservation = TArray<FKronosReservation>();
	NewHostReservation.Emplace(InReservation);

	return SetHostReservations(NewHostReservation);
}

bool UKronosReservationManager::SetHostReservations(const TArray<FKronosReservation>& InReservations)
{
	for (const FKronosReservation& Res : InReservations)
	{
		if (!Res.IsValid(false))
		{
			UE_LOG(LogKronos, Error, TEXT("KronosReservationManager: Failed to set host reservations - a reservation was invalid."));
			return false;
		}
	}

	HostReservations = InReservations;
	return true;
}

TArray<FKronosReservation> UKronosReservationManager::CopyRegisteredReservations(bool bCleanupReservations) const
{
	// Make sure that we have a reservation host beacon.
	if (!IsReservationHost())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosReservationManager: CopyRegisteredReservations was called but we are not a reservation host!"));
		return TArray<FKronosReservation>();
	}

	// If we don't want to do any cleanup, just return the current list of registered reservation.
	if (!bCleanupReservations)
	{
		return ReservationBeaconHost->GetReservations();
	}

	// Loop through the current list of registered reservations and clean up any issues.
	TArray<FKronosReservation> ValidReservations;
	for (const FKronosReservation& Res : ReservationBeaconHost->GetReservations())
	{
		// If the reservation has no members than there is nothing we can do.
		// In theory this will never happen since reservations with zero members are removed automatically.
		if (Res.ReservationMembers.Num() == 0)
		{
			continue;
		}

		// Loop through each member of the reservation and filter out any invalid or pending members.
		TArray<FKronosReservationMember> ValidReservationMembers;
		for (const FKronosReservationMember& ResMember : Res.ReservationMembers)
		{
			if (ResMember.IsValid() && ResMember.bIsCompleted)
			{
				ValidReservationMembers.Add(ResMember);
			}
		}

		// If the reservation has a valid owner, than he remains the owner of the new reservation as well.
		if (Res.ReservationOwner.IsValid())
		{
			FKronosReservation NewReservation;
			NewReservation.ReservationOwner = Res.ReservationOwner;
			NewReservation.ReservationMembers = ValidReservationMembers;

			ValidReservations.Add(NewReservation);
		}

		// The reservation doesn't have a valid owner, meaning they joined as a party but their party leader has left the game.
		// We'll put each member into a new reservation as if they joined the game solo without a party.
		else
		{
			for (const FKronosReservationMember& ValidResMember : ValidReservationMembers)
			{
				FKronosReservation NewReservation;
				NewReservation.ReservationOwner = ValidResMember.PlayerId;
				NewReservation.ReservationMembers.Emplace(ValidResMember);

				ValidReservations.Add(NewReservation);
			}
		}
	}

	return ValidReservations;
}

bool UKronosReservationManager::CompleteReservation(const FUniqueNetIdRepl& PlayerId)
{
	if (IsReservationHost())
	{
		return ReservationBeaconHost->CompleteReservation(PlayerId);
	}

	return false;
}

bool UKronosReservationManager::PlayerHasReservation(const FUniqueNetIdRepl& PlayerId) const
{
	if (IsReservationHost())
	{
		return ReservationBeaconHost->PlayerHasReservation(PlayerId);
	}

	return false;
}

void UKronosReservationManager::DumpReservations() const
{
	if (!IsReservationHost())
	{
		UE_LOG(LogKronos, Warning, TEXT("DumpReservations failed because player is not a reservation host."));
		return;
	}

	ReservationBeaconHost->DumpReservations();
}

FKronosReservation UKronosReservationManager::MakeReservationForPrimaryPlayer()
{
	FKronosReservation Reservation;
	Reservation.ReservationOwner = GetWorld()->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl();
	Reservation.ReservationMembers.Emplace(Reservation.ReservationOwner);

	return Reservation;
}

FKronosReservation UKronosReservationManager::MakeReservationForParty()
{
	FKronosReservation Reservation;
	Reservation.ReservationOwner = GetWorld()->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl();

	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	if (PartyManager->IsInParty())
	{
		// Add every player in the party to the reservation. This includes the party host (aka the reservation owner) as well.
		for (AKronosPartyPlayerState* PartyPlayer : PartyManager->GetPartyPlayerStates())
		{
			const FUniqueNetIdRepl& PlayerId = PartyPlayer->UniqueId;
			if (PlayerId.IsValid())
			{
				Reservation.ReservationMembers.Emplace(PlayerId);
			}
		}
	}

	else
	{
		// We are not in a party, so we just add ourselves to the reservation.
		Reservation.ReservationMembers.Emplace(Reservation.ReservationOwner);
	}

	return Reservation;
}

FKronosReservation UKronosReservationManager::MakeReservationForGamePlayers()
{
	FKronosReservation Reservation;
	Reservation.ReservationOwner = GetWorld()->GetGameInstance()->GetPrimaryPlayerUniqueIdRepl();

	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState)
	{
		if (GameState->PlayerArray.Num() > 0)
		{
			// Add every player in the match to the reservation. This includes the match host (aka the reservation owner) as well.
			for (APlayerState* Player : GameState->PlayerArray)
			{
				const FUniqueNetIdRepl& PlayerId = Player->GetUniqueId();
				if (PlayerId.IsValid())
				{
					Reservation.ReservationMembers.Emplace(PlayerId);
				}
			}
		}

		else
		{
			// Safety measure. In case the player array is empty, we'll add ourselves to the reservation directly.
			// This should be treated as an error though. Maybe this function was called too early?
			UE_LOG(LogKronos, Warning, TEXT("KronosReservationManager: PlayerArray in GameState is empty! Only the local player was added to the reservation."));
			Reservation.ReservationMembers.Emplace(Reservation.ReservationOwner);
		}
	}

	return Reservation;
}

void UKronosReservationManager::RegisterHostReservations()
{
	// Make sure that the primary player always has a proper reservation.
	if (HostReservations.Num() == 0)
	{
		const FKronosReservation& PlayerRes = MakeReservationForPrimaryPlayer();
		HostReservations.Emplace(PlayerRes);
	}

	for (const FKronosReservation& Res : HostReservations)
	{
		ReservationBeaconHost->RegisterReservation(Res);
	}
	
	// Reset the array because these are single use only. 
	HostReservations.Empty();
}

void UKronosReservationManager::OnGameModeInitialized(AGameModeBase* GameMode)
{
	// Initialize a reservation host beacon if the session uses reservations.

	if (GetWorld()->GetNetMode() == NM_Standalone) return;

	UKronosOnlineSession* KronosOnlineSession = UKronosOnlineSession::Get(this);
	if (KronosOnlineSession)
	{
		EOnlineSessionState::Type SessionState = KronosOnlineSession->GetSessionState(NAME_GameSession);
		if (SessionState != EOnlineSessionState::NoSession)
		{
			int32 bUseReservations = 0;
			KronosOnlineSession->GetSessionSetting(NAME_GameSession, SETTING_USERESERVATIONS, bUseReservations);

			if (bUseReservations != 0)
			{
				if (!UGameplayStatics::HasOption(GameMode->OptionsString, TEXT("MaxPlayers")))
				{
					UE_LOG(LogKronos, Warning, TEXT("KronosReservationManager: No 'MaxPlayers' travel option was given. Reservation max players configuration might be wrong!"));
				}

				const int32 MaxReservations = GameMode->GameSession->MaxPlayers;
				InitReservationBeaconHost(MaxReservations);
			}
		}
	}
}

void UKronosReservationManager::OnGameModePreLogin(AGameModeBase* GameMode, const FUniqueNetIdRepl& NewPlayer, FString& ErrorMessage)
{
	// Check if the joining player has a reservation.

	if (IsReservationHost())
	{
		UKronosOnlineSession* KronosOnlineSession = UKronosOnlineSession::Get(this);
		if (KronosOnlineSession)
		{
			if (KronosOnlineSession->IsPlayerBannedFromSession(NAME_GameSession, *NewPlayer.GetUniqueNetId()))
			{
				UE_LOG(LogKronos, Warning, TEXT("KronosReservationManager: Login rejected for player: %s - Player is banned from the session."), *NewPlayer.ToDebugString());
				ErrorMessage = TEXT("PlayerBannedFromSession");
			}
		}

		if (!ReservationBeaconHost->PlayerHasReservation(NewPlayer))
		{
			UE_LOG(LogKronos, Warning, TEXT("KronosReservationManager: Login rejected for player: %s - Player had no reservation."), *NewPlayer.ToDebugString());
			ErrorMessage = TEXT("NoReservation");
		}
	}
}

void UKronosReservationManager::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	// Complete the reservation of the player who joined the game.

	if (IsReservationHost())
	{
		const FUniqueNetIdRepl& NewPlayerId = NewPlayer->PlayerState->GetUniqueId();
		ReservationBeaconHost->CompleteReservation(NewPlayerId);
	}
}

void UKronosReservationManager::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
{
	// Remove the reservation of the player who is leaving the game.

	if (IsReservationHost())
	{
		// Do nothing if the leaving controller is just the debug camera.
		// I don't really understand why the logout event is getting called for it to be honest.
		if (Exiting->IsA<ADebugCameraController>())
		{
			return;
		}

		const FUniqueNetIdRepl& LeavingPlayerId = Exiting->PlayerState->GetUniqueId();
		ReservationBeaconHost->RemoveReservation(LeavingPlayerId);
	}
}

UWorld* UKronosReservationManager::GetWorld() const
{
	// We don't care about the CDO because it's not relevant to world checks.
	// If we don't return nullptr here then blueprints are going to complain.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	return GetOuter()->GetWorld();
}
