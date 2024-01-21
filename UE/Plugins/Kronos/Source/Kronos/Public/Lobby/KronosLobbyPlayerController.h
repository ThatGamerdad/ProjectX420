// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lobby/KronosLobbyPlayerControllerBase.h"
#include "KronosLobbyPlayerController.generated.h"

/**
 * PlayerController class to be paired with other lobby classes.
 */
UCLASS()
class KRONOS_API AKronosLobbyPlayerController : public AKronosLobbyPlayerControllerBase
{
	GENERATED_BODY()
	
protected:

	/**
	 * Whether we want to override the view target when AutoManageActiveCameraTarget() is called.
	 * This means that a camera actor placed in the map will become the view target. By default, the first available camera will be used.
	 * This only works if bAutoManageActiveCameraTarget is enabled in the PlayerController.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby", meta = (EditCondition = "bAutoManageActiveCameraTarget"))
	bool bOverrideViewTarget = true;

	/** Whether we want to find a specific camera when overriding the view target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby", meta = (EditCondition = "bAutoManageActiveCameraTarget && bOverrideViewTarget"))
	bool bFindCameraByTag = false;

	/** The tag that we'll be looking for on camera actors when overriding the view target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby", meta = (EditCondition = "bAutoManageActiveCameraTarget && bOverrideViewTarget && bFindCameraByTag"))
	FName CameraActorTag = NAME_None;

	/**
	 * Whether we are about to travel to a different map or not.
	 * Keeping track of this because we don't want to override the view target when changing maps.
	 */
	bool bLeavingLobby = false;

protected:

	/**
	 * Called when we are overriding the view target.
	 *
	 * @return The actor that will be used as the view target. Returning nullptr is also valid. In that case, the suggested view target will be used.
	 */
	virtual AActor* FindViewTargetOverride();

public:

	// ~ Begin APlayerController Interface
	virtual void AutoManageActiveCameraTarget(AActor* SuggestedTarget) override;
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;
	// ~ End APlayerController Interface
};
