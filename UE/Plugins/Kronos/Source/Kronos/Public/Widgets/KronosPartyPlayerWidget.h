// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KronosPartyPlayerWidget.generated.h"

/**
 * Widget for an existing player in a party.
 */
UCLASS()
class KRONOS_API UKronosPartyPlayerWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	/** Player state of the owning player. */
	UPROPERTY(Transient)
	class AKronosPartyPlayerState* OwningPlayerState;

public:

	/** Initializes the player widget. */
	virtual void InitPlayerWidget(AKronosPartyPlayerState* InOwningPlayerState);

	/** Get the owning player's unique id. */
	UFUNCTION(BlueprintPure, Category = "Default")
	FUniqueNetIdRepl& GetPlayerUniqueId() const;

	/** Get the owning player's name. */
	UFUNCTION(BlueprintPure, Category = "Default")
	FText GetPlayerName() const;

	/** Get the owning player's current elo score. */
	UFUNCTION(BlueprintPure, Category = "Default")
	int32 GetPlayerElo() const;

	/** Get the owning player's current player data. */
	UFUNCTION(BlueprintPure, Category = "Default")
	TArray<int32> GetPlayerData() const;

	/** Whether the owning player is the party leader or not. */
	UFUNCTION(BlueprintPure, Category = "Default")
	bool IsPartyLeader() const;

protected:

	/** Called when the player elo changes (including the first time it replicates). */
	virtual void OnPlayerEloChanged(const int32& PlayerElo);

	/** Called when the player data changes (including the first time it replicates). */
	virtual void OnPlayerDataChanged(const TArray<int32>& PlayerData);

	/** Called when the party leader changes (including the first time it replicates). */
	virtual void OnPartyLeaderChanged(const FUniqueNetIdRepl& UniqueId);

protected:

	/**
	 * Event when the player widget is initialized. Called after the native OnInitialized() event.
	 * At this point the owning player is set, and relevant events are bound.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Widget Initialized")
	void K2_OnPlayerWidgetInitialized();

	/** Event when the player elo changes (including the first time it replicates). */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Elo Changed")
	void K2_OnPlayerEloChanged(const int32& PlayerElo);

	/** Event when the player data changes (including the first time it replicates). */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Player Data Changed")
	void K2_OnPlayerDataChanged(const TArray<int32>& PlayerData);

	/** Event when the party leader changes (including the first time it replicates). */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Party Leader Changed")
	void K2_OnPartyLeaderChanged();
};
