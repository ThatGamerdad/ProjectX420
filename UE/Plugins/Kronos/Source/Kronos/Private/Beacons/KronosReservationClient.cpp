// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Beacons/KronosReservationClient.h"
#include "Beacons/KronosReservationHost.h"
#include "KronosReservationManager.h"
#include "Kronos.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "TimerManager.h"

bool AKronosReservationClient::RequestReservation(const FKronosSearchResult& InSession, const FKronosReservation& InReservation, const FOnKronosReservationRequestComplete& CompletionDelegate)
{
	if (!InReservation.IsValid())
	{
		CompletionDelegate.ExecuteIfBound(InSession, EKronosReservationCompleteResult::ReservationInvalid);
		return false;
	}

	if (InSession.IsValid())
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem)
		{
			IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
			if (SessionInterface.IsValid())
			{
				FString ConnectString;
				if (SessionInterface->GetResolvedConnectString(InSession.OnlineResult, NAME_BeaconPort, ConnectString))
				{
					FURL ConnectURL = FURL(nullptr, *ConnectString, TRAVEL_Absolute);
					if (InitClient(ConnectURL))
					{
						UE_LOG(LogKronos, Log, TEXT("KronosReservationClient: Client initialized. Connecting..."));

						DestSession = InSession;
						PendingReservation = InReservation;

						bReservationRequestPending = true;
						ReservationRequestCompleteDelegate = CompletionDelegate;

						return true;
					}

					else UE_LOG(LogKronos, Error, TEXT("KronosReservationClient: Client failed to initialize."));
				}

				else UE_LOG(LogKronos, Error, TEXT("KronosReservationClient: Failed to resolve connection with string with desired session."));
			}
		}
	}

	CompletionDelegate.ExecuteIfBound(InSession, EKronosReservationCompleteResult::ConnectionError);
	return false;
}

bool AKronosReservationClient::CancelReservation(const FOnCancelKronosReservationComplete& CompletionDelegate)
{
	if (bWasCanceled)
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosReservationClient: Reservation already canceled, or being canceled."));
		return false;
	}

	UE_LOG(LogKronos, Log, TEXT("KronosReservationClient: Canceling reservation..."));

	bWasCanceled = true;

	bCancelReservationPending = true;
	CancelReservationCompleteDelegate = CompletionDelegate;

	bReservationRequestPending = false;
	ReservationRequestCompleteDelegate.Unbind();

	if (GetConnectionState() == EBeaconConnectionState::Open)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_TimeoutCancelReservation, this, &ThisClass::OnCancelReservationTimeout, REQUEST_TIMEOUT, false);

		ServerCancelReservation(PendingReservation);
		return true;
	}

	else
	{
		SignalCancelReservationRequestComplete(true);
		return true;
	}
}

void AKronosReservationClient::OnConnected()
{
	Super::OnConnected();

	if (bWasCanceled)
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosReservationClient: Client connected but the reservation request was canceled."));
		return;
	}

	UE_LOG(LogKronos, Log, TEXT("KronosReservationClient: Client connected. Requesting reservation..."));

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_TimeoutReservationRequest, this, &ThisClass::OnRequestReservationTimeout, REQUEST_TIMEOUT, false);

	ServerRequestReservation(PendingReservation);
}

void AKronosReservationClient::ServerRequestReservation_Implementation(const FKronosReservation& Reservation)
{
	AKronosReservationHost* BeaconHost = Cast<AKronosReservationHost>(GetBeaconOwner());
	if (BeaconHost)
	{
		BeaconHost->ProcessReservationRequest(this, Reservation);
	}
}

void AKronosReservationClient::ClientReceiveReservationResponse_Implementation(EKronosReservationCompleteResult Result)
{
	if (bReservationRequestPending)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TimeoutReservationRequest);
		SignalReservationRequestComplete(DestSession, Result);
	}
}

void AKronosReservationClient::ServerCancelReservation_Implementation(const FKronosReservation& Reservation)
{
	AKronosReservationHost* BeaconHost = Cast<AKronosReservationHost>(GetBeaconOwner());
	if (BeaconHost)
	{
		BeaconHost->ProcessCancelReservation(this, Reservation);
	}
}

void AKronosReservationClient::ClientCancelReservationComplete_Implementation()
{
	if (bCancelReservationPending)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TimeoutCancelReservation);
		SignalCancelReservationRequestComplete(true);
	}
}

void AKronosReservationClient::OnRequestReservationTimeout()
{
	if (bReservationRequestPending)
	{
		SignalReservationRequestComplete(DestSession, EKronosReservationCompleteResult::ConnectionError);
	}
}

void AKronosReservationClient::OnCancelReservationTimeout()
{
	if (bCancelReservationPending)
	{
		SignalCancelReservationRequestComplete(false);
	}
}

void AKronosReservationClient::SignalReservationRequestComplete(const FKronosSearchResult& SearchResult, const EKronosReservationCompleteResult Result)
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosReservationClient: ReservationRequestComplete with result: %s"), LexToString(Result));

	bReservationRequestPending = false;
	ReservationRequestCompleteDelegate.ExecuteIfBound(DestSession, Result);
}

void AKronosReservationClient::SignalCancelReservationRequestComplete(const bool bWasSuccessful)
{
	UE_LOG(LogKronos, Verbose, TEXT("KronosReservationClient: CancelReservationRequest complete with result: %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failure"));

	bCancelReservationPending = false;
	CancelReservationCompleteDelegate.ExecuteIfBound(bWasSuccessful);
}

void AKronosReservationClient::OnFailure()
{
	Super::OnFailure();

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TimeoutReservationRequest);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TimeoutCancelReservation);

	if (bReservationRequestPending)
	{
		SignalReservationRequestComplete(DestSession, EKronosReservationCompleteResult::ConnectionError);
	}

	if (bCancelReservationPending)
	{
		SignalCancelReservationRequestComplete(false);
	}
}

void AKronosReservationClient::DestroyBeacon()
{
	ReservationRequestCompleteDelegate.Unbind();
	CancelReservationCompleteDelegate.Unbind();

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TimeoutReservationRequest);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TimeoutCancelReservation);

	Super::DestroyBeacon();
}
