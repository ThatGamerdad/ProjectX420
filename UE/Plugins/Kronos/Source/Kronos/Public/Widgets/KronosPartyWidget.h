// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KronosPartyWidget.generated.h"

class UDynamicEntryBox;
class AKronosPartyPlayerState;

/**
 * Widget that represents the state of the party for a player.
 */
UCLASS()
class KRONOS_API UKronosPartyWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Whether a player widget should be created for the local player or not. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Party")
	bool bCreateEntryForLocalPlayer = true;

protected:

	/** Dynamic entry box that handles player widgets. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Default")
	UDynamicEntryBox* PartyPlayerEntryBox;

	/** Dynamic entry box that handles open party slot widgets. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Default")
	UDynamicEntryBox* PartySlotEntryBox;

public:

	/** Creates a player widget for the given player. */
	virtual void CreatePlayerWidget(AKronosPartyPlayerState* OwningPlayerState);

	/** Removes the player widget of an existing player. */
	virtual void RemovePlayerWidget(const FUniqueNetIdRepl& PlayerId);

	/** Recreates open party slot widgets. */
	virtual void RecreatePartySlotWidgets();

protected:

	/** Called when we joined a party. */
	UFUNCTION()
	virtual void OnConnectedToParty();

	/** Called when we left a party. */
	UFUNCTION()
	virtual void OnDisconnectedFromParty();

	/** Called when a new player state is added to the party. */
	UFUNCTION()
	virtual void OnPlayerStateAdded(AKronosPartyPlayerState* PlayerState);

	/** Called when an existing player state is being removed from the party. */
	UFUNCTION()
	virtual void OnPlayerStateRemoved(AKronosPartyPlayerState* PlayerState);

	/** Called when a new player joins the party. */
	UFUNCTION()
	virtual void OnPlayerJoinedParty(const FText& DisplayName, const FUniqueNetIdRepl& PlayerId);

	/** Called when an existing player leaves the party. */
	UFUNCTION()
	virtual void OnPlayerLeftParty(const FUniqueNetIdRepl& PlayerId);

	/** Called when a chat message is received. */
	UFUNCTION()
	virtual void OnChatMessageReceived(const FUniqueNetIdRepl& SenderId, const FString& Msg);

protected:

	/** Event when we joined a party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Connected To Party")
	void K2_OnConnectedToParty();

	/** Event when we left a party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Disconnected From Party")
	void K2_OnDisconnectedFromParty();

	/** Event when a new player joins the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Joined Party")
	void K2_OnPlayerJoinedParty(const FText& DisplayName, const FUniqueNetIdRepl& PlayerId);

	/** Event when an existing player leaves the party. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Left Party")
	void K2_OnPlayerLeftParty(const FUniqueNetIdRepl& PlayerId);

	/** Event when a chat message is received. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Chat Message Received")
	void K2_OnChatMessageReceived(const FUniqueNetIdRepl& SenderId, const FString& Msg);

public:
	
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface
};
