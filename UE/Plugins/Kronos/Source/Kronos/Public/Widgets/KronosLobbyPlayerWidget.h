// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KronosLobbyPlayerWidget.generated.h"

class AKronosLobbyPlayerState;
class AKronosLobbyPawn;

/**
 * Widget for an existing player in a lobby.
 */
UCLASS()
class KRONOS_API UKronosLobbyPlayerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Player state of the owning player. */
	UPROPERTY(Transient)
	AKronosLobbyPlayerState* OwningPlayerState;

public:

	/** Initializes the player widget. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void InitPlayerWidget(AKronosLobbyPlayerState* InOwningPlayerState);

	/** Get the player state of the owning player. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual AKronosLobbyPlayerState* GetOwningLobbyPlayerState();

	/** Get the player pawn of the owning player. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual AKronosLobbyPawn* GetOwningLobbyPawn();

protected:

	/** Called when the owning player's name is changed. */
	UFUNCTION()
	virtual void OnPlayerNameChanged(const FString& PlayerName);

	/** Called when the owning player's ready state is changed. */
	UFUNCTION()
	virtual void OnPlayerIsReadyChanged(bool bIsReady);

	/** Called when the owning player is leaving the match. */
	UFUNCTION()
	virtual void OnPlayerDisconnecting();

protected:

	/**
	 * Event when the player widget is initialized. Called after the native OnInitialized() event.
	 * At this point the owning player is set, and relevant events are bound.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Widget Initialized")
	void K2_OnPlayerWidgetInitialized();

	/** Event when the owning player's name changes (including the first time it replicates). */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Name Changed")
	void K2_OnPlayerNameChanged(const FString& PlayerName);

	/** Event when the owning player's ready state changes. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Is Ready Changed")
	void K2_OnPlayerIsReadyChanged(bool bIsReady);

	/** Event when the owning player is leaving the match. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Disconnecting")
	void K2_OnPlayerDisconnecting();
};
