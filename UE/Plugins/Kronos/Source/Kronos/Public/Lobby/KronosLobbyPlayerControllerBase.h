// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KronosLobbyPlayerControllerBase.generated.h"

/**
 * Base PlayerController class to be used for KronosLobbyPlayerController.
 * This class exposes key functions to blueprints without the lobby specific stuff.
 * Blueprint projects can inherit from this class to implement a PlayerController for game matches.
 */
UCLASS()
class KRONOS_API AKronosLobbyPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

public:

	/** Send a game chat message to all players. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void SendChatMessage(const FString& Msg);

	/** Tell the server to send a game chat message to all players. */
	UFUNCTION(Server, Reliable)
	virtual void ServerSendChatMessage(const FString& Msg);

	/** Replicate the game chat message to the client. */
	UFUNCTION(Client, Reliable)
	virtual void ClientReceiveChatMessage(const FString& SenderName, const FString& Msg);

protected:

	/**
	 * Event when a game chat message is received.
	 * 
	 * @param SenderName Name of the player sending the message.
	 * @param Msg The message that was received.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "Client Receive Chat Message")
	void K2_ClientReceiveChatMessage(const FString& SenderName, const FString& Msg);

	/**
	 * Event when the client is being returned to the main menu gracefully.
	 * For general disconnect handling, use the OnCleanupSession event of KronosOnlineSession class.
	 *
	 * @param ReturnReason Text detailing the reason behind the return.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Client Return To Main Menu With Reason")
	void K2_ClientReturnToMainMenuWithTextReason(const FText& ReturnReason);

public:

	// ~ Begin APlayerController Interface
	virtual void ClientReturnToMainMenuWithTextReason_Implementation(const FText& ReturnReason) override;
	// ~ End APlayerController Interface
};
