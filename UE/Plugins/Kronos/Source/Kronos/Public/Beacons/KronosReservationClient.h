// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "KronosTypes.h"
#include "KronosReservationClient.generated.h"

/** Time in seconds before abandoning the reservation request after the connection has been established. */
#define REQUEST_TIMEOUT 5.0f

/**
 * Delegate triggered when a reservation request is complete.
 *
 * @param SearchResult The session the reservation was requested for.
 * @param Result The reservation result.
 */
DECLARE_DELEGATE_TwoParams(FOnKronosReservationRequestComplete, const FKronosSearchResult& /** SearchResult */, const EKronosReservationCompleteResult /** Result */);

/**
 * Delegate triggered when a reservation cancel request is complete.
 *
 * @param bWasSuccessful Whether the reservation was canceled successfully.
 */
DECLARE_DELEGATE_OneParam(FOnCancelKronosReservationComplete, const bool /** bWasSuccessful */);

/**
 * A beacon client used for making reservations with an existing game session.
 * Intentionally not using the built-in APartyBeaconClient class because even though it's also for making reservations, it is designed for use with master servers.
 */
UCLASS()
class KRONOS_API AKronosReservationClient : public AOnlineBeaconClient
{
	GENERATED_BODY()

protected:

	/** The session the reservation was requested for. */
	FKronosSearchResult DestSession;

	/** The reservation that is requested with the session. */
	FKronosReservation PendingReservation;

	/** Has the reservation requested been canceled. */
	bool bWasCanceled;

	/** Whether a reservation request is in progress. */
	bool bReservationRequestPending;

	/** Whether a reservation cancel request is in progress. */
	bool bCancelReservationPending;

	/** Handle used to time-out a reservation request. */
	FTimerHandle TimerHandle_TimeoutReservationRequest;

	/** Handle used to time-out a reservation cancel request. */
	FTimerHandle TimerHandle_TimeoutCancelReservation;

private:

	/** Reservation request complete delegate. */
	FOnKronosReservationRequestComplete ReservationRequestCompleteDelegate;

	/** Reservation cancel request complete delegate. */
	FOnCancelKronosReservationComplete CancelReservationCompleteDelegate;

public:

	/**
	 * Sends a request to the remote host to make reservations for the given players in the remote host's session.
	 * NOTE: This operation is async.
	 *
	 * @param InSession Session to reserve for.
	 * @param InReservation Reservation to request.
	 * @param CompletionDelegate Delegate triggered when the reservation request is complete.
	 *
	 * @return True if the request has been sent, false otherwise.
	 */
	virtual bool RequestReservation(const FKronosSearchResult& InSession, const FKronosReservation& InReservation, const FOnKronosReservationRequestComplete& CompletionDelegate = FOnKronosReservationRequestComplete());

	/**
	 * Sends a request to the remote host to cancel the pending reservation that was requested by this beacon.
	 * NOTE: This operation is async.
	 *
	 * @param CompletionDelegate Delegate triggered when the reservation request is complete.
	 *
	 * @return True if the request has been sent, false otherwise.
	 */
	virtual bool CancelReservation(const FOnCancelKronosReservationComplete& CompletionDelegate = FOnCancelKronosReservationComplete());

	/** @return The pending reservation that has been requested. */
	const FKronosReservation& GetPendingReservation() const { return PendingReservation; }

protected:

	/** Tell the server to make a reservation. */
	UFUNCTION(Server, Reliable)
	virtual void ServerRequestReservation(const FKronosReservation& Reservation);

	/** Response from the server after making a reservation request. */
	UFUNCTION(Client, Reliable)
	virtual void ClientReceiveReservationResponse(EKronosReservationCompleteResult Result);

	/** Tell the server to cancel a reservation. */
	UFUNCTION(Server, Reliable)
	virtual void ServerCancelReservation(const FKronosReservation& Reservation);

	/** Response from the server after making a reservation cancel request. */
	UFUNCTION(Client, Reliable)
	virtual void ClientCancelReservationComplete();

	friend class AKronosReservationHost;

protected:

	/** Called when a reservation was requested but no response was received. */
	virtual void OnRequestReservationTimeout();

	/** Called when a reservation canceling was requested but no response was received. */
	virtual void OnCancelReservationTimeout();

	/** Triggers reservation request complete delegate. */
	virtual void SignalReservationRequestComplete(const FKronosSearchResult& SearchResult, const EKronosReservationCompleteResult Result);

	/** Triggers reservation cancel request complete delegate. */
	virtual void SignalCancelReservationRequestComplete(const bool bWasSuccessful);

public:

	//~ Begin AOnlineBeaconClient Interface
	virtual void OnConnected() override;
	virtual void OnFailure() override;
	virtual void DestroyBeacon() override;
	//~ End AOnlineBeaconClient Interface
};
