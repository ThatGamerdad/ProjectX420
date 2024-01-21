// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KronosTypes.h"
#include "KronosReservationManager.generated.h"

class AKronosReservationListener;
class AKronosReservationHost;
class AGameModeBase;
class AController;
class APlayerController;

/**
 * KronosReservationManager is responsible for handling the reservation beacons of the user.
 * It is automatically spawned and managed by the KronosOnlineSession class.
 * 
 * Online Beacons are used to establish a lightweight connection between players without a normal game connection.
 * They allow us to connect to a server without traveling to it first. In this case, the match host.
 * 
 * Please note that this system doesn't handle reservation client beacons.
 * Those are handled by the active KronosMatchmakingPolicy.
 * 
 * @see AKronosReservationListener
 * @see AKronosReservationHost
 * @see AKronosReservationClient
 */
UCLASS(Blueprintable, BlueprintType)
class KRONOS_API UKronosReservationManager : public UObject
{
	GENERATED_BODY()

protected:

	/** Reservation listener beacon. Only valid while we are hosting a game session that requires reservations to join. */
	UPROPERTY(Transient)
	AKronosReservationListener* ReservationBeaconListener;

	/** Reservation host beacon. Only valid while we are hosting a game session that requires reservations to join. */
	UPROPERTY(Transient)
	AKronosReservationHost* ReservationBeaconHost;

	/** Reservations that should automatically be registered when creating a reservation host beacon. */
	TArray<FKronosReservation> HostReservations;

public:

	/** Get the reservation manager from the KronosOnlineSession. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Get Kronos Reservation Manager", meta = (WorldContext = "WorldContextObject"))
	static UKronosReservationManager* Get(const UObject* WorldContextObject);

	/**
	 * Initialize the KronosReservationManager during game startup.
	 * Called by the KronosOnlineSession.
	 */
	virtual void Initialize();

	/**
	 * Deinitialize the KronosReservationManager before game shutdown.
	 * Called by the KronosOnlineSession.
	 */
	virtual void Deinitialize() {};

public:

	/**
	 * Initializes a reservation host beacon. Players who wish to join the session will connect to this beacon to request a reservation.
	 * Should be called after a game session has been created and the match has been hosted.
	 */
	virtual bool InitReservationBeaconHost(const int32 MaxReservations);

	/**
	 * Reconfigures the reservation capacity of the beacon.
	 * Allows the server to change reservation parameters when the playlist configuration is changed.
	 */
	virtual bool ReconfigureMaxReservations(const int32 InMaxReservations);

	/** Destroys all reservation beacons. */
	virtual void DestroyReservationBeacons();

	/**
	 * Sets the host reservations to a single reservation. This reservation will be registered automatically when creating a reservation host beacon.
	 * This can be used to preemptively make a reservation for our party before hosting a custom game for example.
	 */
	virtual bool SetHostReservation(const FKronosReservation& InReservation);

	/**
	 * Sets the host reservations. These reservation will be registered automatically when creating a reservation host beacon.
	 * This can be used to preemptively make a reservation for our party before hosting a custom game for example.
	 */
	virtual bool SetHostReservations(const TArray<FKronosReservation>& InReservations);

	/**
	 * Get a copy of the current list of registered reservations, optionally cleaning up invalid reservations.
	 * If cleanup is enabled, invalid reservations will not be included and reservations without a valid owner will have their members put in their own reservations instead. 
	 * Intended to be used as the host reservation before traveling to a new map in a match.
	 */
	virtual TArray<FKronosReservation> CopyRegisteredReservations(bool bCleanupReservations = true) const;

	/**
	 * Manually complete the reservation of the given player. This means that the player has arrived at the session, and no longer needs to be timed out.
	 * Since game mode Login events are not called after seamless travel (e.g. from lobby to match), reservations cannot be completed automatically by the plugin.
	 * If the game uses reservations during matches make sure to call this function after a player has finished seamless traveling.
	 * Only needed for seamless traveling players.
	 */
	virtual bool CompleteReservation(const FUniqueNetIdRepl& PlayerId);

	/** @return Whether the given player has a reservation or not. */
	virtual bool PlayerHasReservation(const FUniqueNetIdRepl& PlayerId) const;

	/** @return Whether we are a host for reservation requests. */
	virtual bool IsReservationHost() const { return ReservationBeaconHost != nullptr; }

	/** @return The reservation listener beacon. */
	AKronosReservationListener* GetListenerBeacon() const { return ReservationBeaconListener; }

	/** @return The reservation host beacon. */
	AKronosReservationHost* GetHostBeacon() const { return ReservationBeaconHost; }

	/** Dump reservations to the console. */
	virtual void DumpReservations() const;

public:

	/**
	 * Makes a reservation that includes the local player.
	 * Structure only! The reservation still has to be registered.
	 */
	virtual FKronosReservation MakeReservationForPrimaryPlayer();

	/**
	 * Makes a reservation that includes all party members.
	 * Works even if the player is not in a party. In that case only the local player will be included.
	 * Structure only! The reservation still has to be registered.
	 */
	virtual FKronosReservation MakeReservationForParty();

	/**
	 * Makes a reservation that includes all players in the current match.
	 * All players will be put in the same reservation. In general, using CopyRegisteredReservations is better since it keeps reservations as they are.
	 * Works both in standalone and multiplayer scenarios since it uses the PlayerArray of the GameState to construct the reservation.
	 * Structure only! The reservation still has to be registered.
	 */
	virtual FKronosReservation MakeReservationForGamePlayers();

protected:

	/** Registers the host reservation with the host beacon. */
	virtual void RegisterHostReservations();

	/** Entry point when the game mode has been initialized for the recently loaded map. */
	virtual void OnGameModeInitialized(AGameModeBase* GameMode);

	/** Entry point when a client first contacts the server. */
	virtual void OnGameModePreLogin(AGameModeBase* GameMode, const FUniqueNetIdRepl& NewPlayer, FString& ErrorMessage);

	/** Entry point when a player joins the game as well as after non-seamless ServerTravel. */
	virtual void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);

	/** Entry point when a player leaves the game as well as during non-seamless ServerTravel. */
	virtual void OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting);

public:

	//~ Begin UObject interface
	virtual UWorld* GetWorld() const final; // Required by blueprints to have access to gameplay statics and other world context based nodes.
	//~ End UObject interface
};
