// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Lobby/KronosLobbyPlayerController.h"
#include "Kronos.h"
#include "Camera/CameraActor.h"
#include "UObject/UObjectIterator.h"
#include "Engine/GameViewportClient.h"

void AKronosLobbyPlayerController::AutoManageActiveCameraTarget(AActor* SuggestedTarget)
{
	// Check if we want to override the view target.
	if (bOverrideViewTarget && !bLeavingLobby)
	{
		AActor* ViewTargetOverride = FindViewTargetOverride();
		if (ViewTargetOverride)
		{
			// We have a new view target, override the suggested target.
			SuggestedTarget = ViewTargetOverride;
		}
	}

	Super::AutoManageActiveCameraTarget(SuggestedTarget);
}

AActor* AKronosLobbyPlayerController::FindViewTargetOverride()
{
	for (TObjectIterator<ACameraActor> It; It; ++It)
	{
		ACameraActor* CameraActor = *It;
		if (CameraActor)
		{
			if (bFindCameraByTag && !CameraActor->ActorHasTag(CameraActorTag))
			{
				// Camera doesn't have the tag that we are looking for.
				continue;
			}

			// Found a good view target.
			return CameraActor;
		}
	}

	UE_CLOG(bFindCameraByTag, LogKronos, Warning, TEXT("No Camera found with tag '%s'"), *CameraActorTag.ToString());
	UE_LOG(LogKronos, Warning, TEXT("Failed to override view target."));
	return nullptr;
}

void AKronosLobbyPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	bLeavingLobby = true;

	// Because of seamless travel, widgets will stay on the screen even after changing maps.
	// We don't want this however, so we manually remove all widgets.
	if (bIsSeamlessTravel)
	{
		if (UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport())
		{
			ViewportClient->RemoveAllViewportWidgets();
		}
	}
}
