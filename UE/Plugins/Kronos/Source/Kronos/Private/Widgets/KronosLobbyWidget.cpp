// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Widgets/KronosLobbyWidget.h"
#include "Lobby/KronosLobbyGameMode.h"
#include "Lobby/KronosLobbyGameState.h"
#include "Lobby/KronosLobbyPlayerState.h"
#include "Kronos.h"
#include "TimerManager.h"

void UKronosLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	WaitInitialReplication();
}

AKronosLobbyGameState* UKronosLobbyWidget::GetLobbyGameState()
{
	AKronosLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AKronosLobbyGameState>();
	if (LobbyGameState)
	{
		return LobbyGameState;
	}

	UE_LOG(LogKronos, Error, TEXT("KronosLobbyWidget: Failed to get lobby game state."));
	return nullptr;
}

AKronosLobbyPlayerState* UKronosLobbyWidget::GetLocalPlayerState()
{
	AKronosLobbyPlayerState* LobbyPlayerState = GetOwningPlayer()->GetPlayerState<AKronosLobbyPlayerState>();
	if (LobbyPlayerState)
	{
		return LobbyPlayerState;
	}

	UE_LOG(LogKronos, Error, TEXT("KronosLobbyWidget: Failed to get local player state."));
	return nullptr;
}

bool UKronosLobbyWidget::GetPlayerHasAuthority()
{
	return GetWorld()->GetNetMode() < NM_Client;
}

void UKronosLobbyWidget::WaitInitialReplication()
{
	AGameStateBase* GameState = GetWorld()->GetGameState();
	APlayerState* PlayerState = GetOwningPlayer()->PlayerState;

	// Check if necessary objects are available.
	if (GameState && PlayerState)
	{
		OnLobbyWidgetInitialized();
		return;
	}

	// We are still waiting for initial object replication. Checking again in the next frame...
	else
	{
		if (IsValid(this))
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::WaitInitialReplication);
			return;
		}
	}
	
}

void UKronosLobbyWidget::OnLobbyWidgetInitialized()
{
	K2_OnLobbyWidgetInitialized();

	AKronosLobbyGameState* LobbyState = GetLobbyGameState();
	if (LobbyState)
	{
		LobbyState->OnPlayerConnectedToLobby.AddDynamic(this, &ThisClass::OnPlayerJoinedLobby);
		LobbyState->OnPlayerDisconnectedFromLobby.AddDynamic(this, &ThisClass::OnPlayerLeftLobby);
		LobbyState->OnLobbyUpdated.AddDynamic(this, &ThisClass::OnLobbyUpdated);

		// Auto receive an update so that UI elements are set properly from the start.
		OnLobbyUpdated(LobbyState->GetLobbyState(), LobbyState->GetLobbyCountdownTime());
	}

	AKronosLobbyPlayerState* LobbyPlayerState = GetLocalPlayerState();
	if (LobbyPlayerState)
	{
		LobbyPlayerState->OnLobbyPlayerIsReadyChanged.AddDynamic(this, &ThisClass::OnPlayerIsReadyChanged);

		// Auto receive an update so that UI elements are set properly from the start.
		OnPlayerIsReadyChanged(LobbyPlayerState->GetPlayerIsReady());
	}
}

void UKronosLobbyWidget::OnPlayerJoinedLobby(AKronosLobbyPlayerState* PlayerState)
{
	K2_OnPlayerJoinedLobby(PlayerState);
}

void UKronosLobbyWidget::OnPlayerLeftLobby(AKronosLobbyPlayerState* PlayerState)
{
	K2_OnPlayerLeftLobby(PlayerState);
}

void UKronosLobbyWidget::OnLobbyUpdated(const EKronosLobbyState LobbyState, const int32 LobbyCountdownTime)
{
	K2_OnLobbyUpdated(LobbyState, LobbyCountdownTime);

	if (LobbyState < EKronosLobbyState::StartingMatch)
	{
		// If the lobby was previously in the 'StartingMatch' state, notify the widget that the starting was canceled.
		if (bStartingMatch)
		{
			bStartingMatch = false;
			OnStartingMatchCanceled();
		}
	}

	if (LobbyState == EKronosLobbyState::StartingMatch)
	{
		bStartingMatch = true;
		OnStartingMatch();
	}
}

void UKronosLobbyWidget::OnPlayerIsReadyChanged(bool bIsReady)
{
	K2_OnPlayerIsReadyChanged(bIsReady);
}

void UKronosLobbyWidget::OnStartingMatch()
{
	K2_OnStartingMatch();
}

void UKronosLobbyWidget::OnStartingMatchCanceled()
{
	K2_OnStartingMatchCanceled();
}
