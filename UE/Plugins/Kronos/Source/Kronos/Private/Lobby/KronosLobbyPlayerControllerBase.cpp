// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Lobby/KronosLobbyPlayerControllerBase.h"
#include "Kronos.h"
#include "GameFramework/PlayerState.h"

void AKronosLobbyPlayerControllerBase::SendChatMessage(const FString& Msg)
{
	// Make sure that we are on the server.
	if (!HasAuthority())
	{
		ServerSendChatMessage(Msg);
		return;
	}

	// Make sure that we have a valid player state.
	if (!PlayerState)
	{
		UE_LOG(LogKronos, Error, TEXT("Failed to send chat message - PlayerState is null."));
		return;
	}

	// Get the name of the local player.
	const FString& SenderName = PlayerState->GetPlayerName();

	// Send the chat message to all players.
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AKronosLobbyPlayerControllerBase* KronosPlayerController = Cast<AKronosLobbyPlayerControllerBase>(Iterator->Get());
		if (KronosPlayerController)
		{
			KronosPlayerController->ClientReceiveChatMessage(SenderName, Msg);
		}
	}
}

void AKronosLobbyPlayerControllerBase::ServerSendChatMessage_Implementation(const FString& Msg)
{
	SendChatMessage(Msg);
}

void AKronosLobbyPlayerControllerBase::ClientReceiveChatMessage_Implementation(const FString& SenderName, const FString& Msg)
{
	K2_ClientReceiveChatMessage(SenderName, Msg);
}

void AKronosLobbyPlayerControllerBase::ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason)
{
	// Give blueprint a chance to implement custom logic before returning to the main menu.
	K2_ClientReturnToMainMenuWithTextReason(ReturnReason);

	Super::ClientReturnToMainMenuWithTextReason_Implementation(ReturnReason);
}
