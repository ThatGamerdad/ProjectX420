// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "KronosTypes.h"
#include "KronosReservationHost.generated.h"

class AKronosReservationClient;

/**
 * A beacon host used for taking reservations for an existing game session.
 * Intentionally not using the built-in APartyBeaconHost class because even though it's also for taking reservations, it is designed for use with dedicated servers.
 */
UCLASS()
class KRONOS_API AKronosReservationHost : public AOnlineBeaconHostObject
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	AKronosReservationHost(const FObjectInitializer& ObjectInitializer);

protected:

	/** Reservation capacity. */
	int32 MaxNumReservations;

	/** Registered reservations for the session. */
	TArray<FKronosReservation> Reservations;

public:

	/**
	 * Initializes the reservation host beacon.
	 * This can be overridden to extend the system when required (e.g. take reservations for two teams).
	 */
	virtual bool InitHostBeacon(const int32 InMaxReservations);

	/**
	 * Reconfigures the reservation capacity of the beacon.
	 * Allows the server to change reservation parameters when the playlist configuration is changed.
	 */
	virtual bool ReconfigureMaxReservations(const int32 InMaxReservations);

	/** Handle a reservation request received from an incoming client. */
	virtual void ProcessReservationRequest(AKronosReservationClient* Client, const FKronosReservation& InReservation);

	/** Attempts to register a new reservation. */
	virtual EKronosReservationCompleteResult RegisterReservation(const FKronosReservation& InReservation);

	/** Handle a reservation cancel request received from an existing client. */
	virtual void ProcessCancelReservation(AKronosReservationClient* Client, const FKronosReservation& InReservation);

	/** Attempts to remove an existing reservation. */
	virtual bool RemoveReservation(const FUniqueNetIdRepl& PlayerId);

	/** Completes the reservation of a given player. This means that the player has arrived in the session. */
	virtual bool CompleteReservation(const FUniqueNetIdRepl& PlayerId);

	/** @return Whether the given player has a reservation or not. */
	virtual bool PlayerHasReservation(const FUniqueNetIdRepl& PlayerId) const;

	/**
	 * Attempt to find an existing reservation.
	 * 
	 * @param OutPlayerReservation The reservation of the player. Only valid if the reservation was found!
	 * @param OutOwningReservation The owning reservation of the player's reservation. Only valid if the reservation was found!
	 * 
	 * @return True if the reservation was found. False otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Default", meta = (ReturnDisplayName = "Success"))
	virtual bool FindReservation(const FUniqueNetIdRepl& PlayerId, FKronosReservationMember& OutPlayerReservation, FKronosReservation& OutOwningReservation);

	/** Dumps current reservations to the console. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void DumpReservations() const;

	/** Max number of reservations that can be consumed across all parties. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetMaxNumReservations() const;

	/** Number of reservation entries inside the beacon (*NOT* the number of consumed reservations, there are members inside these entries). */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetNumReservations() const;

	/** Number of reservations actually used/consumed across all parties inside the beacon (sum of reservation members). */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual int32 GetNumConsumedReservations() const;

	/** Get all registered reservations. */
	UFUNCTION(BlueprintPure, Category = "Default")
	const TArray<FKronosReservation>& GetReservations() const { return Reservations; }

protected:

	/**
	 * Called when this host beacon is initialized by the reservation manager.
	 * The HostReservations will be registered immediately after this function.
	 */
	virtual void OnInitialized();

	/** Called when a new reservation is registered. */
	virtual void OnReservationRegistered(const FKronosReservation& NewReservation);

	/** Called when an existing reservation is removed. */
	virtual void OnReservationRemoved(const FUniqueNetIdRepl& PlayerId);

	/**
	 * Called before a reservation's owner is removed.
	 * Can be used to notify reservation members that the owner is leaving (e.g. leave and reconnect to the party), or to assign a new owner to the reservation.
	 * OnReservationRemoved will be called right after this function for the reservation owner player.
	 */
	virtual void PreReservationOwnerRemoved(const FUniqueNetIdRepl& OwnerId, const FKronosReservation& Reservation);

	/** Called when a reservation isn't completed in time. Most likely the player hasn't arrived at the session. */
	UFUNCTION()
	virtual void TimeoutReservation(const FUniqueNetIdRepl& PlayerId);

	friend class UKronosReservationManager;

protected:

	/**
	 * Event when this host beacon is initialized by the reservation manager.
	 * The HostReservations will be registered immediately after this event.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Default", DisplayName = "On Initialized")
	void K2_OnInitialized();

	/**
	 * Called before a reservation is registered.
	 * If this function doesn't return EKronosReservationCompleteResult::ReservationAccepted then the reservation won't be registered.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Default", DisplayName = "Pre Register Reservation")
	EKronosReservationCompleteResult K2_PreRegisterReservation(const FKronosReservation& InReservation);

	/** Event when a new reservation is registered. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Reservation Registered")
	void K2_OnReservationRegistered(const FKronosReservation& NewReservation);

	/** Event when an existing reservation is removed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Reservation Removed")
	void K2_OnReservationRemoved(const FUniqueNetIdRepl& PlayerId);

	/**
	 * Event before a reservation's owner is removed.
	 * Can be used to notify reservation members that the owner is leaving (e.g. leave and reconnect to the party), or to assign a new owner to the reservation.
	 * OnReservationRemoved will be called right after this event for the reservation owner player.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "Pre Reservation Owner Removed")
	void K2_PreReservationOwnerRemoved(const FUniqueNetIdRepl& OwnerId, const FKronosReservation& Reservation);

public:

	//~ Begin AOnlineBeaconHostObject Interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AOnlineBeaconHostObject Interface
};
