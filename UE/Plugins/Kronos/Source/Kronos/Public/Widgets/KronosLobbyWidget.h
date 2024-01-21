// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KronosLobbyWidget.generated.h"

class AKronosLobbyGameState;
class AKronosLobbyPlayerState;
enum class EKronosLobbyState : uint8;

/**
 * Widget that represents the lobby for a player.
 */
UCLASS()
class KRONOS_API UKronosLobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Get the lobby state. */
	UFUNCTION(BlueprintPure, Category = "Default")
	AKronosLobbyGameState* GetLobbyGameState();

	/** Get the local player's state. */
	UFUNCTION(BlueprintPure, Category = "Default")
	AKronosLobbyPlayerState* GetLocalPlayerState();

	/** Whether the local player has server authority or not. */
	UFUNCTION(BlueprintPure, Category = "Default")
	bool GetPlayerHasAuthority();

protected:

	/**
	 * Whether the lobby is before or after the 'StartingMatch' state.
	 * Keeping track of this in case the starting doesn't go through and gets canceled.
	 */
	bool bStartingMatch;

protected:

	/** Check if all necessary objects have been replicated. If not, the check will be performed again in the next frame. */
	virtual void WaitInitialReplication();

	/**
	 * Called when the lobby widget is initialized. Called after the native OnInitialized() event.
	 * At this point both the GameState and PlayerState have replicated and relevant events are bound.
	 */
	virtual void OnLobbyWidgetInitialized();

	/** Called when a new player joins the lobby. */
	UFUNCTION()
	virtual void OnPlayerJoinedLobby(AKronosLobbyPlayerState* PlayerState);

	/** Called when an existing player leaves the lobby. */
	UFUNCTION()
	virtual void OnPlayerLeftLobby(AKronosLobbyPlayerState* PlayerState);

	/** Called when the lobby state changes. */
	UFUNCTION()
	virtual void OnLobbyUpdated(const EKronosLobbyState LobbyState, const int32 LobbyCountdownTime);

	/** Called when the local player's ready state changes. */
	UFUNCTION()
	virtual void OnPlayerIsReadyChanged(bool bIsReady);

	/** Called when the match is starting. */
	virtual void OnStartingMatch();

	/** Called when the match was starting but it was canceled for some reason. */
	virtual void OnStartingMatchCanceled();

protected:

	/**
	 * Event when the lobby widget is initialized. Called after the native OnInitialized() event.
	 * At this point both the GameState and PlayerState have replicated and relevant events are bound.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Lobby Widget Initialized")
	void K2_OnLobbyWidgetInitialized();

	/** Event when a new player joins the lobby. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Joined Lobby")
	void K2_OnPlayerJoinedLobby(AKronosLobbyPlayerState* PlayerState);

	/** Event when an existing player leaves the lobby. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Left Lobby")
	void K2_OnPlayerLeftLobby(AKronosLobbyPlayerState* PlayerState);

	/** Event when the lobby state changes. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Lobby Updated")
	void K2_OnLobbyUpdated(const EKronosLobbyState LobbyState, const int32 LobbyCountdownTime);

	/** Event when the local player's ready state changes. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Is Ready Changed")
	void K2_OnPlayerIsReadyChanged(bool bIsReady);

	/** Event when the match is starting. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Starting Match")
	void K2_OnStartingMatch();

	/** Event when the match was starting but it was canceled for some reason. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Starting Match Canceled")
	void K2_OnStartingMatchCanceled();

public:

	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface
};
