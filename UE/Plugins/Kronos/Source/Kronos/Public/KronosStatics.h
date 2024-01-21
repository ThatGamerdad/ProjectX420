// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KronosTypes.h"
#include "KronosStatics.generated.h"

class UKronosOnlineSession;
class AKronosReservationHost;
class AKronosPartyHost;
class AKronosPartyClient;
class AKronosPartyState;
class AKronosPartyPlayerState;
class APlayerState;

/**
 * Static class implementing blueprint access for key Kronos features and classes.
 * Some features of the plugin are implemented via blueprint proxies (e.g. start matchmaking, update session, etc.).
 */
UCLASS()
class KRONOS_API UKronosStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Check whether the user is authenticated or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|User", meta = (WorldContext = "WorldContextObject"))
	static bool IsAuthenticated(const UObject* WorldContextObject);

	/** Check whether the user is currently logged in or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|User", meta = (WorldContext = "WorldContextObject"))
	static bool IsLoggedIn(const UObject* WorldContextObject);

	/** Get the local player's unique id. */
	UFUNCTION(BlueprintPure, Category = "Kronos|User", meta = (WorldContext = "WorldContextObject"))
	static FUniqueNetIdRepl GetLocalPlayerId(const UObject* WorldContextObject);

	/** Get the local player's nickname. */
	UFUNCTION(BlueprintPure, Category = "Kronos|User", meta = (WorldContext = "WorldContextObject"))
	static FString GetPlayerNickname(const UObject* WorldContextObject);

public:

	/** Check whether matchmaking is in progress or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Matchmaking", meta = (WorldContext = "WorldContextObject"))
	static bool IsMatchmaking(const UObject* WorldContextObject);
	
	/** Get the current matchmaking state. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Matchmaking", meta = (WorldContext = "WorldContextObject"))
	static EKronosMatchmakingState GetMatchmakingState(const UObject* WorldContextObject);

	/** Get the result of the matchmaking. Only valid after the matchmaking has been completed. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Matchmaking", meta = (WorldContext = "WorldContextObject"))
	static EKronosMatchmakingCompleteResult GetMatchmakingResult(const UObject* WorldContextObject);

	/** Get the reason behind the matchmaking failure. Only valid after matchmaking has resulted in failure. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Matchmaking", meta = (WorldContext = "WorldContextObject"))
	static EKronosMatchmakingFailureReason GetMatchmakingFailureReason(const UObject* WorldContextObject);

	/** Get the search results of the latest matchmaking pass. */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Matchmaking", meta = (WorldContext = "WorldContextObject"))
	static TArray<FKronosSearchResult> GetMatchmakingSearchResults(const UObject* WorldContextObject);

	/** Returns true if the host params are valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Matchmaking", DisplayName = "Is Valid Host Params", meta = (ScriptMethod = "IsValid"))
	static bool IsHostParamsValid(const FKronosHostParams& HostParams) { return HostParams.IsValid(false); }

	/** Returns true if the matchmaking params are valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Matchmaking", DisplayName = "Is Valid Matchmaking Params", meta = (ScriptMethod = "IsValid"))
	static bool IsMatchmakingParamsValid(const FKronosMatchmakingParams& MatchmakingParams) { return MatchmakingParams.IsValid(false); }

	/** Returns true if the search params are valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Matchmaking", DisplayName = "Is Valid Search Params", meta = (ScriptMethod = "IsValid"))
	static bool IsSearchParamsValid(const FKronosSearchParams& SearchParams) { return SearchParams.IsValid(false); }

	/** Returns true if the specific session query params are valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Matchmaking", DisplayName = "Is Valid Specific Session Query", meta = (ScriptMethod = "IsValid"))
	static bool IsSpecificSessionQueryValid(const FKronosSpecificSessionQuery& SpecificSessionQuery) { return SpecificSessionQuery.IsValid(); }

public:

	/**
	 * ServerTravel to a new map. Should only be called in a networked environment.
	 * Additional travel options can be given via the '?' delimiter.
	 * Example travel URL to a map in "Maps" folder: /Game/Maps/MyMap?MyTravelOption
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Game", meta = (WorldContext = "WorldContextObject"))
	static void ServerTravelToLevel(const UObject* WorldContextObject, FString TravelURL);

	/** Start the current match. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Game", meta = (WorldContext = "WorldContextObject"))
	static void StartMatch(const UObject* WorldContextObject);

	/** End the current match. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Game", meta = (WorldContext = "WorldContextObject"))
	static void EndMatch(const UObject* WorldContextObject);

	/** Kick the given player from the match. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Game", meta = (WorldContext = "WorldContextObject"))
	static bool KickPlayerFromMatch(const UObject* WorldContextObject, APlayerController* KickedPlayer, bool bBanFromSession = false);

	/** Ban the given player from the match. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Game", meta = (WorldContext = "WorldContextObject"))
	static bool BanPlayerFromMatch(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Check whether the given player is banned from the match or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Game", meta = (WorldContext = "WorldContextObject"))
	static bool IsPlayerBannedFromMatch(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

public:

	/**
	 * Set the host reservations to a single reservation. This reservation will be registered automatically when creating a reservation host beacon.
	 * This can be used to preemptively make a reservation for our party before hosting a custom game for example.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static bool SetHostReservation(const UObject* WorldContextObject, const FKronosReservation& Reservation);

	/**
	 * Set the host reservations. These reservations will be registered automatically when creating a reservation host beacon.
	 * This can be used to preemptively make a reservation for our party before hosting a custom game for example.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static bool SetHostReservations(const UObject* WorldContextObject, const TArray<FKronosReservation>& Reservations);

	/**
	 * Get a copy of the current list of registered reservations, optionally cleaning up invalid reservations.
	 * If cleanup is enabled, invalid reservations will not be included and reservations without a valid owner will have their members put in their own reservations instead.
	 * Intended to be used as the host reservation before traveling to a new map in a match.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static TArray<FKronosReservation> CopyRegisteredReservations(const UObject* WorldContextObject, bool bCleanupReservations = true);

	/**
	 * Reconfigure the reservation capacity of the host beacon.
	 * Allows the server to change reservation parameters when the playlist configuration is changed.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static bool ReconfigureMaxReservations(const UObject* WorldContextObject, const int32 MaxReservations);

	/**
	 * Manually complete the reservation of the given player. This means that the player has arrived at the session, and no longer needs to be timed out.
	 * Since Login events are not called after seamless travel (e.g. from lobby to match), reservations cannot be completed automatically.
	 * If the game uses reservations during matches make sure to call this function after a player has finished seamless traveling.
	 * Only needed for seamless traveling players.
	 *
	 * For blueprint projects, the GameMode's OnSwapPlayerControllers event is a good place for this.
	 * 
	 * @see GameModeBase::OnSwapPlayerControllers
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static bool CompleteReservation(const UObject* WorldContextObject, const APlayerController* PlayerController);

	/**
	 * Manually complete the reservation of the given player. This means that the player has arrived at the session, and no longer needs to be timed out.
	 * Since Login events are not called after seamless travel (e.g. from lobby to match), reservations cannot be completed automatically.
	 * If the game uses reservations during matches make sure to call this function after a player has finished seamless traveling.
	 * Only needed for seamless traveling players.
	 *
	 * For blueprint projects, the GameMode's OnSwapPlayerControllers event is a good place for this.
	 * 
	 * @see GameModeBase::OnSwapPlayerControllers
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static bool CompleteReservationById(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Whether we are a host for reservation requests. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static bool IsReservationHost(const UObject* WorldContextObject);

	/** Whether the given player has a reservation or not. */
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static bool PlayerHasReservation(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Get the reservation host beacon. Only valid on the server. */
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static AKronosReservationHost* GetReservationHost(const UObject* WorldContextObject);

	/**
	 * Create a new 'FKronosReservation' that includes only the local player.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static FKronosReservation MakeReservationForPrimaryPlayer(const UObject* WorldContextObject);

	/**
	 * Create a new 'FKronosReservation' that includes all party members.
	 * Works even if the player is not in a party. In that case only the local player will be included.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static FKronosReservation MakeReservationForParty(const UObject* WorldContextObject);

	/**
	 * Create a new 'FKronosReservation' that includes all players in the current match.
	 * All players will be put in the same reservation. In general, using CopyRegisteredReservations is better since it keeps reservations as they are.
	 * Works both in standalone and multiplayer scenarios since it uses the PlayerArray of the GameState to construct the reservation.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Reservation", meta = (WorldContext = "WorldContextObject"))
	static FKronosReservation MakeReservationForGamePlayers(const UObject* WorldContextObject);

	/** Returns true if the reservation owner and all reservation members are valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Reservation", DisplayName = "Is Valid Reservation", meta = (ScriptMethod = "IsValid"))
	static bool IsReservationValid(const FKronosReservation& Reservation) { return Reservation.IsValid(false); }

	/** Returns true if the reservation is valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Reservation", DisplayName = "Is Valid Reservation Member", meta = (ScriptMethod = "IsValid"))
	static bool IsReservationMemberValid(const FKronosReservationMember& ReservationMember) { return ReservationMember.IsValid(); }

public:

	/** Kick the given player from the party. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static void KickPlayerFromParty(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId, bool bBanFromSession = false);

	/** Ban the given player from the party. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static bool BanPlayerFromParty(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Check whether we are in a party or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static bool IsInParty(const UObject* WorldContextObject);

	/** Check whether we are a party leader or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static bool IsPartyLeader(const UObject* WorldContextObject);

	/** Check whether all connected clients are logged in to the party. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static bool IsEveryClientInParty(const UObject* WorldContextObject);

	/** Check whether the given player is banned from the party or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static bool IsPlayerBannedFromParty(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Check whether the party leader is matchmaking or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static bool IsPartyLeaderMatchmaking(const UObject* WorldContextObject);

	/** Returns the number of players in the party, or 1 if not in a party. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static int32 GetPartySize(const UObject* WorldContextObject);

	/** Get the number of players in the party. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static int32 GetNumPlayersInParty(const UObject* WorldContextObject);

	/** Get the max number of players in the party. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static int32 GetMaxNumPlayersInParty(const UObject* WorldContextObject);

	/** Get the average elo score of the party. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static int32 GetPartyEloAverage(const UObject* WorldContextObject);

	/** Get the party host beacon. Only valid on the server while hosting a party. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static AKronosPartyHost* GetPartyHost(const UObject* WorldContextObject);

	/** Get a specific player's party client actor. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static AKronosPartyClient* GetPartyClient(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Get the party state. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static AKronosPartyState* GetPartyState(const UObject* WorldContextObject);

	/** Get a specific player in the party. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", DisplayName = "Get Party Client State", meta = (WorldContext = "WorldContextObject"))
	static AKronosPartyPlayerState* GetPartyPlayerState(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Get all players in the party. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", DisplayName = "Get Party Client States", meta = (WorldContext = "WorldContextObject"))
	static TArray<AKronosPartyPlayerState*> GetPartyPlayerStates(const UObject* WorldContextObject);

	/** Get all party players unique id. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", DisplayName = "Get Party Client Unique Ids", meta = (WorldContext = "WorldContextObject"))
	static TArray<FUniqueNetIdRepl> GetPartyPlayerUniqueIds(const UObject* WorldContextObject);

	/** Check whether we have information about a party that we were a part of previously. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static bool HasLastPartyInfo(const UObject* WorldContextObject);

	/**
	 * Get the cached information about the last party that we were a part of.
	 * Updated when we create or join a party successfully.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Party", meta = (WorldContext = "WorldContextObject"))
	static FKronosLastPartyInfo GetLastPartyInfo(const UObject* WorldContextObject);

public:

	/**
	 * Get a specific friend from the cached friends list.
	 * Only valid after reading friends list!
	 * An additional list name can be provided for the online subsystem for further processing.
	 * Steam doesn't make use of this list, while EOS can use "onlinePlayers" and "inGamePlayers" names to filter friends.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Friend", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "ListName"))
	static FKronosOnlineFriend GetFriend(const UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, const FString& ListName = TEXT(""));

	/**
	 * Get the number of friends contained in the given cached friends list.
	 * Only valid after reading friends list!
	 */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Friend", meta = (WorldContext = "WorldContextObject"))
	virtual int32 GetFriendCount(const UObject* WorldContextObject, bool bInGamePlayersOnly) const;

	/**
	 * Get whether the local player is friends with the given player.
	 * An additional list name can be provided for the online subsystem for further processing.
	 * Steam doesn't make use of this list, while EOS can use "onlinePlayers" and "inGamePlayers" names to filter friends.      
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Friend", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "ListName"))
	static bool IsFriend(const UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, const FString& ListName = TEXT(""));

	/** Send a session invite to the given friend to join the local player's game. */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Friend", meta = (WorldContext = "WorldContextObject"))
	static bool SendGameInviteToFriend(const UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId);

	/** Send a session invite to the given friend to join the local player's party. */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Friend", meta = (WorldContext = "WorldContextObject"))
	static bool SendPartyInviteToFriend(const UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId);

	/** Returns true if the online friend is valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Friends", DisplayName = "Is Valid Online Friend", meta = (ScriptMethod = "IsValid"))
	static bool IsOnlineFriendValid(const FKronosOnlineFriend& Friend) { return Friend.IsValid(); }

public:

	/** Show the online subsystem's external invite UI for game sessions if supported (e.g. Steam Overlay). */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Kronos|UI")
	static bool ShowGameInviteUI();

	/** Show the online subsystem's external invite UI for party sessions if supported (e.g. Steam Overlay). */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Kronos|UI")
	static bool ShowPartyInviteUI();

	/** Show the online subsystem's external profile UI of the given player if supported (e.g. Steam Overlay). */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Kronos|UI", meta = (WorldContext = "WorldContextObject"))
	static bool ShowProfileUI(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

public:

	/** Get the session settings of the current game session. */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Session", meta = (WorldContext = "WorldContextObject"))
	static bool GetGameSessionSettings(const UObject* WorldContextObject, FKronosSessionSettings& SessionSettings);

	/** Get the session settings of the current game session. */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Session", meta = (WorldContext = "WorldContextObject"))
	static bool GetPartySessionSettings(const UObject* WorldContextObject, FKronosSessionSettings& SessionSettings);

	/**
	 * Get a specific session setting from the current game session.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Game Session Setting (int32)", meta = (ReturnDisplayName = "Success", WorldContext = "WorldContextObject"))
	static bool GetGameSessionSettingInt32(const UObject* WorldContextObject, FName Key, int32& Value);

	/**
	 * Get a specific session setting from the current game session.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Game Session Setting (FString)", meta = (ReturnDisplayName = "Success", WorldContext = "WorldContextObject"))
	static bool GetGameSessionSettingFString(const UObject* WorldContextObject, FName Key, FString& Value);

	/**
	 * Get a specific session setting from the current game session.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Game Session Setting (float)", meta = (ReturnDisplayName = "Success", WorldContext = "WorldContextObject"))
	static bool GetGameSessionSettingFloat(const UObject* WorldContextObject, FName Key, float& Value);

	/**
	 * Get a specific session setting from the current game session.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Game Session Setting (bool)", meta = (ReturnDisplayName = "Success", WorldContext = "WorldContextObject"))
	static bool GetGameSessionSettingBool(const UObject* WorldContextObject, FName Key, bool& Value);

	/**
	 * Get a specific session setting from the current party session.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Party Session Setting (int32)", meta = (ReturnDisplayName = "Success", WorldContext = "WorldContextObject"))
	static bool GetPartySessionSettingInt32(const UObject* WorldContextObject, FName Key, int32& Value);

	/**
	 * Get a specific session setting from the current party session.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Party Session Setting (FString)", meta = (ReturnDisplayName = "Success", WorldContext = "WorldContextObject"))
	static bool GetPartySessionSettingFString(const UObject* WorldContextObject, FName Key, FString& Value);

	/**
	 * Get a specific session setting from the current party session.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Party Session Setting (float)", meta = (ReturnDisplayName = "Success", WorldContext = "WorldContextObject"))
	static bool GetPartySessionSettingFloat(const UObject* WorldContextObject, FName Key, float& Value);

	/**
	 * Get a specific session setting from the current party session.
	 *
	 * @param WorldContextObject The world context object in which this call is taking place.
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Party Session Setting (bool)", meta = (ReturnDisplayName = "Success", WorldContext = "WorldContextObject"))
	static bool GetPartySessionSettingBool(const UObject* WorldContextObject, FName Key, bool& Value);

public:

	/** Returns true if the search result is valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", meta = (ScriptMethod = "IsValid"))
	static bool IsSessionValid(const FKronosSearchResult& SearchResult) { return SearchResult.IsValid(); }

	/** Get the search result's type. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", meta = (ScriptMethod))
	static FName GetSessionType(const FKronosSearchResult& SearchResult) { return SearchResult.GetSessionType(); }

	/** Get the search result's unique id. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", meta = (ScriptMethod))
	static FUniqueNetIdRepl GetSessionUniqueId(const FKronosSearchResult& SearchResult);

	/** Get the search result's owning player id. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", meta = (ScriptMethod))
	static FUniqueNetIdRepl GetSessionOwnerUniqueId(const FKronosSearchResult& SearchResult) { return SearchResult.GetOwnerUniqueId(); }

	/** Get the search result's owning player name. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", meta = (ScriptMethod))
	static FString GetSessionOwnerUsername(const FKronosSearchResult& SearchResult) { return SearchResult.GetOwnerUsername(); }

	/** Get the search result's current player count. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", meta = (ScriptMethod))
	static int32 GetNumPlayersInSession(const FKronosSearchResult& SearchResult) { return SearchResult.GetNumPlayers(); }

	/** Get the session settings of the given search result. */
	UFUNCTION(BlueprintCallable, Category = "Kronos|Session", meta = (ScriptMethod))
	static FKronosSessionSettings GetSessionSettings(const FKronosSearchResult& SearchResult) { return SearchResult.GetSessionSettings(); }

	/**
	 * Get a specific session setting from the given search result.
	 *
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Session Setting (int32)", meta = (ReturnDisplayName = "Success", ScriptMethod))
	static bool GetSessionSettingInt32(const FKronosSearchResult& SearchResult, FName Key, int32& Value) { return SearchResult.GetSessionSetting(Key, Value); }

	/**
	 * Get a specific session setting from the given search result.
	 *
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Session Setting (FString)", meta = (ReturnDisplayName = "Success", ScriptMethod))
	static bool GetSessionSettingFString(const FKronosSearchResult& SearchResult, FName Key, FString& Value) { return SearchResult.GetSessionSetting(Key, Value); }

	/**
	 * Get a specific session setting from the given search result.
	 *
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Session Setting (float)", meta = (ReturnDisplayName = "Success", ScriptMethod))
	static bool GetSessionSettingFloat(const FKronosSearchResult& SearchResult, FName Key, float& Value) { return SearchResult.GetSessionSetting(Key, Value); }

	/**
	 * Get a specific session setting from the given search result.
	 *
	 * @param Key Name of the session setting to get.
	 * @param Value The session settings current value.
	 *
	 * @return Whether the session setting was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Kronos|Session", DisplayName = "Get Session Setting (bool)", meta = (ReturnDisplayName = "Success", ScriptMethod))
	static bool GetSessionSettingBool(const FKronosSearchResult& SearchResult, FName Key, bool& Value) { return SearchResult.GetSessionSetting(Key, Value); }

public:

	/** Create a new 'FKronosSessionSetting' from its members. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Make Kronos Session Setting (int32)", meta = (NativeMakeFunc))
	static FKronosSessionSetting MakeKronosSessionSettingInt32(FName Key, int32 Value, bool bAdvertise = true) { return FKronosSessionSetting(Key, Value, bAdvertise ? EOnlineDataAdvertisementType::ViaOnlineService : EOnlineDataAdvertisementType::DontAdvertise); }

	/** Create a new 'FKronosSessionSetting' from its members. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Make Kronos Session Setting (FString)", meta = (NativeMakeFunc))
	static FKronosSessionSetting MakeKronosSessionSettingFString(FName Key, FString Value, bool bAdvertise = true) { return FKronosSessionSetting(Key, Value, bAdvertise ? EOnlineDataAdvertisementType::ViaOnlineService : EOnlineDataAdvertisementType::DontAdvertise); }

	/** Create a new 'FKronosSessionSetting' from its members. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Make Kronos Session Setting (float)", meta = (NativeMakeFunc))
	static FKronosSessionSetting MakeKronosSessionSettingFloat(FName Key, float Value, bool bAdvertise = true) { return FKronosSessionSetting(Key, Value, bAdvertise ? EOnlineDataAdvertisementType::ViaOnlineService : EOnlineDataAdvertisementType::DontAdvertise); }

	/** Create a new 'FKronosSessionSetting' from its members. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Make Kronos Session Setting (bool)", meta = (NativeMakeFunc))
	static FKronosSessionSetting MakeKronosSessionSettingBool(FName Key, bool Value, bool bAdvertise = true) { return FKronosSessionSetting(Key, Value, bAdvertise ? EOnlineDataAdvertisementType::ViaOnlineService : EOnlineDataAdvertisementType::DontAdvertise); }

	/** Create a new 'FKronosQuerySetting' from its members. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Make Kronos Query Setting (int32)", meta = (NativeMakeFunc))
	static FKronosQuerySetting MakeKronosQuerySettingInt32(FName Key, int32 Value, EKronosQueryComparisonOp Comparison) { return FKronosQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison)); }

	/** Create a new 'FKronosQuerySetting' from its members. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Make Kronos Query Setting (FString)", meta = (NativeMakeFunc))
	static FKronosQuerySetting MakeKronosQuerySettingFString(FName Key, FString Value, EKronosQueryComparisonOp Comparison) { return FKronosQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison)); }

	/** Create a new 'FKronosQuerySetting' from its members. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Make Kronos Query Setting (float)", meta = (NativeMakeFunc))
	static FKronosQuerySetting MakeKronosQuerySettingFloat(FName Key, float Value, EKronosQueryComparisonOp Comparison) { return FKronosQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison)); }

	/** Create a new 'FKronosQuerySetting' from its members. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Make Kronos Query Setting (bool)", meta = (NativeMakeFunc))
	static FKronosQuerySetting MakeKronosQuerySettingBool(FName Key, bool Value, EKronosQueryComparisonOp Comparison) { return FKronosQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison)); }

	/** Create a new 'EKronosMatchmakingFlags' from its possible flags. */
	UFUNCTION(BlueprintPure, Category = "Kronos", meta = (NativeMakeFunc))
	static int32 MakeKronosMatchmakingFlags(const bool bNoHost, const bool bSkipReservation, const bool bSkipEloChecks);

public:

	/** Check whether the current world is being torn down or not. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (WorldContext = "WorldContextObject"))
	static bool IsTearingDownWorld(const UObject* WorldContextObject);

	/** Get the unique id of a player from their player state. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (ScriptMethod))
	static const FUniqueNetIdRepl& GetPlayerUniqueId(const APlayerState* Player);

	/** Get the player state of a player from their unique id. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (WorldContext = "WorldContextObject"))
	static APlayerState* GetPlayerStateFromUniqueId(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Get the player controller of a player from their unique id. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (WorldContext = "WorldContextObject"))
	static APlayerController* GetPlayerControllerFromUniqueId(const UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId);

	/** Returns true if the unique id is valid. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (DisplayName = "Is Valid Unique Id", ScriptMethod = "IsValid"))
	static bool IsUniqueNetIdReplValid(const FUniqueNetIdRepl& UniqueId) { return UniqueId.IsValid(); }

	/** Returns true if A is equal to B (A == B). */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (DisplayName = "Equal (FUniqueNetIdRepl)", CompactNodeTitle = "==", Keywords = "== equal", ScriptOperator = "=="))
	static bool EqualEqual_CompareUniqueNetIdRepl(const FUniqueNetIdRepl& A, const FUniqueNetIdRepl& B) { return A == B; };

	/** Returns true if A is not equal to B (A != B). */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (DisplayName = "Not Equal (FUniqueNetIdRepl)", CompactNodeTitle = "!=", Keywords = "!= not equal", ScriptOperator = "!="))
	static bool NotEqual_CompareUniqueNetIdRepl(const FUniqueNetIdRepl& A, const FUniqueNetIdRepl& B) { return A != B; };

	/** Convert a UniqueNetIdRepl to a string. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (DisplayName = "ToString (FUniqueNetIdRepl)", CompactNodeTitle = "->", Keywords = "cast convert", ScriptMethod = "ToString", BlueprintAutocast))
	static FString Conv_UniqueNetIdReplToString(const FUniqueNetIdRepl& UniqueId) { return UniqueId.ToString(); }

	/** Convert a UniqueNetIdRepl to a string with additional debug information. */
	UFUNCTION(BlueprintPure, Category = "Kronos|Utilities", meta = (DisplayName = "ToDebugString (FUniqueNetIdRepl)", Keywords = "cast convert", ScriptMethod = "ToDebugString"))
	static FString Conv_UniqueNetIdReplToDebugString(const FUniqueNetIdRepl& UniqueId) { return UniqueId.ToDebugString(); }
};
