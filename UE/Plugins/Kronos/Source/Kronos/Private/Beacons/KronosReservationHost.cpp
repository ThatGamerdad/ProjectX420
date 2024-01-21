// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Beacons/KronosReservationHost.h"
#include "Beacons/KronosReservationClient.h"
#include "KronosOnlineSession.h"
#include "KronosConfig.h"
#include "Kronos.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AKronosReservationHost::AKronosReservationHost(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ClientBeaconActorClass = GetDefault<UKronosConfig>()->ReservationClientClass;
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

bool AKronosReservationHost::InitHostBeacon(const int32 InMaxReservations)
{
	MaxNumReservations = InMaxReservations;
	return true;
}

void AKronosReservationHost::OnInitialized()
{
	K2_OnInitialized();
}

bool AKronosReservationHost::ReconfigureMaxReservations(const int32 InMaxReservations)
{
	if (Reservations.Num() > InMaxReservations)
	{
		UE_LOG(LogKronos, Error, TEXT("KronosReservationHost: Failed to reconfigure max reservations. There are more registered reservations than the new max reservation count."));
		return false;
	}

	MaxNumReservations = InMaxReservations;
	return true;
}

void AKronosReservationHost::ProcessReservationRequest(AKronosReservationClient* Client, const FKronosReservation& InReservation)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationHost: Reservation request received. Processing..."));
	UE_LOG(LogKronos, Verbose, TEXT("ReservationOwner: %s NumReservationMembers: %d"), *InReservation.ReservationOwner.ToDebugString(), InReservation.ReservationMembers.Num());

	if (Client)
	{
		EKronosReservationCompleteResult Result = EKronosReservationCompleteResult::UnknownError;
		Result = RegisterReservation(InReservation);

		KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationHost: Reservation processed. Result: %s"), LexToString(Result));

		Client->ClientReceiveReservationResponse(Result);
	}
}

EKronosReservationCompleteResult AKronosReservationHost::K2_PreRegisterReservation_Implementation(const FKronosReservation& InReservation)
{
	return EKronosReservationCompleteResult::ReservationAccepted;
}

EKronosReservationCompleteResult AKronosReservationHost::RegisterReservation(const FKronosReservation& InReservation)
{
	// Check if beacon denying requests.
	if (GetBeaconState() == EBeaconState::DenyRequests)
	{
		return EKronosReservationCompleteResult::ReservationDenied;
	}

	// Check reservation validity.
	if (!InReservation.IsValid())
	{
		return EKronosReservationCompleteResult::ReservationInvalid;
	}

	// Check reservation count.
	if ((GetNumConsumedReservations() + InReservation.ReservationMembers.Num()) > MaxNumReservations)
	{
		return EKronosReservationCompleteResult::ReservationLimitReached;
	}

	// Give blueprints a chance to abort registering the reservation.
	EKronosReservationCompleteResult K2Result = K2_PreRegisterReservation(InReservation);
	if (K2Result != EKronosReservationCompleteResult::ReservationAccepted)
	{
		return K2Result;
	}

	UKronosOnlineSession* KronosOnlineSession = UKronosOnlineSession::Get(this);
	for (const FKronosReservationMember& ResMember : InReservation.ReservationMembers)
	{
		// Check if player is banned from the session.
		if (KronosOnlineSession && KronosOnlineSession->IsPlayerBannedFromSession(NAME_GameSession, *ResMember.PlayerId.GetUniqueNetId()))
		{
			return EKronosReservationCompleteResult::ReservationDenied;
		}

		// Check for duplicate reservations.
		{
			FKronosReservationMember PlayerReservation;
			FKronosReservation OwningReservation;

			if (FindReservation(ResMember.PlayerId, PlayerReservation, OwningReservation))
			{
				// The player already has a reservation, and according to the reservation the player is already in the game.
				if (PlayerReservation.bIsCompleted)
				{
					return EKronosReservationCompleteResult::ReservationDuplicate;
				}

				// The player already has a reservation, but that reservation is still pending meaning the player never actually joined the game.
				// Somehow the player found the same session again, before his previous reservation was timed out.
				// We are going to remove the previous reservation before registering the new one.
				else
				{
					UE_LOG(LogKronos, Log, TEXT("Cleaning up pending duplicate reservation..."))
					RemoveReservation(ResMember.PlayerId);
				}
			}
		}
	}

	// Register the reservation.
	int32 ReservationIdx = Reservations.Add(InReservation);

	// Set reservation timeouts.
	for (FKronosReservationMember& ResMember : Reservations[ReservationIdx].ReservationMembers)
	{
		FTimerDelegate TimeoutDelegate = FTimerDelegate::CreateUFunction(this, TEXT("TimeoutReservation"), ResMember.PlayerId);
		GetWorld()->GetTimerManager().SetTimer(ResMember.TimerHandle_ReservationTimeout, TimeoutDelegate, GetDefault<UKronosConfig>()->ReservationTimeout, false);
	}

	// Notification that a new reservation has been registered.
	OnReservationRegistered(Reservations[ReservationIdx]);

	return EKronosReservationCompleteResult::ReservationAccepted;
}

void AKronosReservationHost::OnReservationRegistered(const FKronosReservation& NewReservation)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationHost: Reservation registered for %d player(s)."), NewReservation.ReservationMembers.Num());
	if (UE_LOG_ACTIVE(LogKronos, Verbose))
	{
		DumpReservations();
	}

	// Give blueprints a chance to implement custom logic.
	K2_OnReservationRegistered(NewReservation);
}

void AKronosReservationHost::ProcessCancelReservation(AKronosReservationClient* Client, const FKronosReservation& InReservation)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationHost: Cancel reservation received. Processing..."));

	if (Client)
	{
		for (const FKronosReservationMember& ReservationMember : InReservation.ReservationMembers)
		{
			RemoveReservation(ReservationMember.PlayerId);
		}

		Client->ClientCancelReservationComplete();
	}
}

bool AKronosReservationHost::RemoveReservation(const FUniqueNetIdRepl& PlayerId)
{
	if (PlayerId.IsValid())
	{
		for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
		{
			FKronosReservation& ReservationEntry = Reservations[ResIdx];
			for (int32 PlayerIdx = 0; PlayerIdx < ReservationEntry.ReservationMembers.Num(); PlayerIdx++)
			{
				if (*ReservationEntry.ReservationMembers[PlayerIdx].PlayerId == PlayerId)
				{
					// Clear reservation timeout for the player since there is no point in waiting for him.
					FTimerHandle& TimeoutHandle = ReservationEntry.ReservationMembers[PlayerIdx].TimerHandle_ReservationTimeout;
					GetWorld()->GetTimerManager().ClearTimer(TimeoutHandle);

					if (ReservationEntry.ReservationOwner == PlayerId)
					{
						// Notification that the reservation's owner is getting removed.
						PreReservationOwnerRemoved(PlayerId, ReservationEntry);

						static FUniqueNetIdRepl EmptyId = FUniqueNetIdRepl();
						ReservationEntry.ReservationOwner = EmptyId;
					}

					// Remove the reservation.
					Reservations[ResIdx].ReservationMembers.RemoveAt(PlayerIdx);
					if (ReservationEntry.ReservationMembers.Num() == 0)
					{
						Reservations.RemoveAt(ResIdx);
					}

					// Notification that the reservation was removed.
					OnReservationRemoved(PlayerId);

					return true;
				}
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosReservationHost: Failed to remove reservation for %s."), *PlayerId.ToDebugString());
	return false;
}

void AKronosReservationHost::OnReservationRemoved(const FUniqueNetIdRepl& PlayerId)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationHost: Reservation removed for %s."), *PlayerId.ToDebugString());
	if (UE_LOG_ACTIVE(LogKronos, Verbose))
	{
		DumpReservations();
	}

	K2_OnReservationRemoved(PlayerId);
}

void AKronosReservationHost::PreReservationOwnerRemoved(const FUniqueNetIdRepl& OwnerId, const FKronosReservation& Reservation)
{
	K2_PreReservationOwnerRemoved(OwnerId, Reservation);
}

bool AKronosReservationHost::CompleteReservation(const FUniqueNetIdRepl& PlayerId)
{
	if (PlayerId.IsValid())
	{
		for (FKronosReservation& ReservationEntry : Reservations)
		{
			for (FKronosReservationMember& ReservationMember : ReservationEntry.ReservationMembers)
			{
				if (ReservationMember.PlayerId == PlayerId)
				{
					ReservationMember.bIsCompleted = true;
					GetWorld()->GetTimerManager().ClearTimer(ReservationMember.TimerHandle_ReservationTimeout);

					KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosReservationHost: Reservation completed for %s."), *PlayerId.ToDebugString());
					if (UE_LOG_ACTIVE(LogKronos, Verbose))
					{
						DumpReservations();
					}

					return true;
				}
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosReservationHost: Failed to complete reservation for %s."), *PlayerId.ToDebugString());
	return false;
}

void AKronosReservationHost::TimeoutReservation(const FUniqueNetIdRepl& PlayerId)
{
	if (PlayerId.IsValid())
	{
		FKronosReservationMember PlayerReservation;
		FKronosReservation OwningReservation;

		if (FindReservation(PlayerId, PlayerReservation, OwningReservation))
		{
			// Make sure that the player hasn't joined yet.
			for (APlayerState* PlayerState : GetWorld()->GetGameState()->PlayerArray)
			{
				if (PlayerState->GetUniqueId() == PlayerId)
				{
					// Player is logged in. No need to remove reservation. We'll also fix the reservation state.
					PlayerReservation.bIsCompleted = true;
					return;
				}
			}

			// Player hasn't joined yet. Revoke his reservation.
			UE_LOG(LogKronos, Log, TEXT("KronosReservationHost: Reservation timed out for %s"), *PlayerId.ToDebugString());
			RemoveReservation(PlayerId);
		}

		else UE_LOG(LogKronos, Warning, TEXT("KronosReservationHost: TimeoutReservation failed - could not find reservation for player: %s"), *PlayerId.ToDebugString());
	}

	else UE_LOG(LogKronos, Error, TEXT("KronosReservationHost: TimeoutReservation failed - invalid player id."));
}

bool AKronosReservationHost::PlayerHasReservation(const FUniqueNetIdRepl& PlayerId) const
{
	if (PlayerId.IsValid())
	{
		for (const FKronosReservation& ReservationEntry : Reservations)
		{
			for (const FKronosReservationMember& ReservationMember : ReservationEntry.ReservationMembers)
			{
				if (ReservationMember.PlayerId == PlayerId)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool AKronosReservationHost::FindReservation(const FUniqueNetIdRepl& PlayerId, FKronosReservationMember& OutPlayerReservation, FKronosReservation& OutOwningReservation)
{
	if (PlayerId.IsValid())
	{
		for (const FKronosReservation& ReservationEntry : Reservations)
		{
			for (const FKronosReservationMember& ReservationMember : ReservationEntry.ReservationMembers)
			{
				if (ReservationMember.PlayerId == PlayerId)
				{
					OutPlayerReservation = ReservationMember;
					OutOwningReservation = ReservationEntry;

					return true;
				}
			}
		}
	}

	return false;
}

void AKronosReservationHost::DumpReservations() const
{
	UE_LOG(LogKronos, Log, TEXT("KronosReservationHost: Dumping reservations..."));

	for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FKronosReservation& ReservationEntry = Reservations[ResIdx];
		UE_LOG(LogKronos, Log, TEXT("[%d] Owner: %s with %d members"), 1 + ResIdx, *ReservationEntry.ReservationOwner.ToDebugString(), ReservationEntry.ReservationMembers.Num());

		for (int32 PlayerIdx = 0; PlayerIdx < ReservationEntry.ReservationMembers.Num(); PlayerIdx++)
		{
			const FKronosReservationMember& ReservationMember = ReservationEntry.ReservationMembers[PlayerIdx];
			UE_LOG(LogKronos, Log, TEXT("    - %s (%s)"), *ReservationMember.PlayerId.ToDebugString(), ReservationMember.bIsCompleted ? TEXT("Completed") : TEXT("Pending"));
		}
	}
}

int32 AKronosReservationHost::GetMaxNumReservations() const
{
	return MaxNumReservations;
}

int32 AKronosReservationHost::GetNumReservations() const
{
	return Reservations.Num();
}

int32 AKronosReservationHost::GetNumConsumedReservations() const
{
	int32 OutNumConsumedReservations = 0;
	for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FKronosReservation& ReservationEntry = Reservations[ResIdx];
		OutNumConsumedReservations += ReservationEntry.ReservationMembers.Num();
	}

	return OutNumConsumedReservations;
}

void AKronosReservationHost::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Make sure that all reservation timeouts have been cleared before the reservation host is destroyed.
	for (FKronosReservation& ReservationEntry : Reservations)
	{
		for (FKronosReservationMember& ReservationMember : ReservationEntry.ReservationMembers)
		{
			FTimerHandle& TimeoutHandle = ReservationMember.TimerHandle_ReservationTimeout;
			GetWorld()->GetTimerManager().ClearTimer(TimeoutHandle);
		}
	}

	Super::EndPlay(EndPlayReason);
}
