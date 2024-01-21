// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "KronosLobbyHUD.generated.h"

class AKronosLobbyPlayerState;

/**
 * HUD class to be paired with other lobby classes.
 */
UCLASS()
class KRONOS_API AKronosLobbyHUD : public AHUD
{
	GENERATED_BODY()

protected:

	/** Check if all necessary objects and values have been replicated. If not, the check will be performed again in the next frame. */
	void WaitInitialReplication();

	/**
	 * Get whether initial replication has finished.
	 * Can be overridden to include additional replicated values that need to replicate before the actor is considered initialized.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Default")
	bool HasInitialReplicationFinished() const;

	/** Called after initial replication has finished. */
	virtual void OnInitialReplicationFinished();

	/** Called when a player joins the lobby. */
	UFUNCTION()
	virtual void OnPlayerJoinedLobby(AKronosLobbyPlayerState* PlayerState);

	/** Called when a player leaves the lobby. */
	UFUNCTION()
	virtual void OnPlayerLeftLobby(AKronosLobbyPlayerState* PlayerState);

protected:

	/**
	 * Event when initial replication has finished for the HUD class.
	 * You can override the HasInitialReplicationFinished() function to add your own requirements.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Initial Replication Finished")
	void K2_OnInitialReplicationFinished();

	/** Event when a player joins the lobby. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Joined Lobby")
	void K2_OnPlayerJoinedLobby(AKronosLobbyPlayerState* PlayerState);

	/** Event when a player leaves the lobby. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Left Lobby")
	void K2_OnPlayerLeftLobby(AKronosLobbyPlayerState* PlayerState);

public:

	//~ Begin AHUD Interface
	virtual void BeginPlay() override;
	//~ End AHUD Interface
};
