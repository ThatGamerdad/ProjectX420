// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Widgets/KronosLobbyPlayerWidget.h"
#include "Lobby/KronosLobbyPlayerState.h"
#include "Lobby/KronosLobbyPawn.h"
#include "Kronos.h"

void UKronosLobbyPlayerWidget::InitPlayerWidget(AKronosLobbyPlayerState* InOwningPlayerState)
{
	if (InOwningPlayerState)
	{
		OwningPlayerState = InOwningPlayerState;

		OwningPlayerState->OnLobbyPlayerNameChanged.AddDynamic(this, &ThisClass::OnPlayerNameChanged);
		OwningPlayerState->OnLobbyPlayerIsReadyChanged.AddDynamic(this, &ThisClass::OnPlayerIsReadyChanged);
		OwningPlayerState->OnLobbyPlayerDisconnecting.AddDynamic(this, &ThisClass::OnPlayerDisconnecting);

		K2_OnPlayerWidgetInitialized();

		// Make sure that we didn't miss the name changed event.
		FString PlayerName = OwningPlayerState->GetPlayerName();
		if (!PlayerName.IsEmpty())
		{
			OnPlayerNameChanged(OwningPlayerState->GetPlayerName());
		}

		// Make sure that we didn't miss the player ready state changed event.
		bool bIsReady = OwningPlayerState->GetPlayerIsReady();
		if (bIsReady)
		{
			OnPlayerIsReadyChanged(bIsReady);
		}
	}

	else UE_LOG(LogKronos, Error, TEXT("KronosLobbyPlayerWidget: Failed to initialize! Owning player state is null."));
}

AKronosLobbyPlayerState* UKronosLobbyPlayerWidget::GetOwningLobbyPlayerState()
{
	if (OwningPlayerState)
	{
		return OwningPlayerState;
	}

	UE_LOG(LogKronos, Error, TEXT("KronosLobbyPlayerWidget: Failed to get owning lobby player state."));
	return nullptr;
}

AKronosLobbyPawn* UKronosLobbyPlayerWidget::GetOwningLobbyPawn()
{
	if (OwningPlayerState)
	{
		APawn* PlayerPawn = OwningPlayerState->GetPawn();
		if (PlayerPawn)
		{
			AKronosLobbyPawn* LobbyPawn = Cast<AKronosLobbyPawn>(PlayerPawn);
			if (LobbyPawn)
			{
				return LobbyPawn;
			}
		}
	}

	UE_LOG(LogKronos, Error, TEXT("KronosLobbyPlayerWidget: Failed to get owning lobby pawn."));
	return nullptr;
}

void UKronosLobbyPlayerWidget::OnPlayerNameChanged(const FString& PlayerName)
{
	K2_OnPlayerNameChanged(PlayerName);
}

void UKronosLobbyPlayerWidget::OnPlayerIsReadyChanged(bool bIsReady)
{
	K2_OnPlayerIsReadyChanged(bIsReady);
}

void UKronosLobbyPlayerWidget::OnPlayerDisconnecting()
{
	K2_OnPlayerDisconnecting();
}
