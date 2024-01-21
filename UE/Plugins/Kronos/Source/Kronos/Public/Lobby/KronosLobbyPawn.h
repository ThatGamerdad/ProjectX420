// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "KronosLobbyPawn.generated.h"

class UCapsuleComponent;
class UArrowComponent;

/**
 * Pawn class to be paired with other lobby classes.
 */
UCLASS()
class KRONOS_API AKronosLobbyPawn : public APawn
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	AKronosLobbyPawn(const FObjectInitializer& ObjectInitializer);

protected:

	/** Pawn collision component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComponent;

	/** Pawn arrow component. */
	UPROPERTY()
	UArrowComponent* ArrowComponent;

protected:

	/** Initialize the pawn's location (in case we want the local player to be in a fixed spot). */
	virtual void InitPawnLocation();

	/** Called when the player data changes for this pawn. */
	UFUNCTION()
	virtual void OnLobbyPlayerDataChanged(const TArray<int32>& PlayerData);

	/** Event when the player data changes for this pawn. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Lobby Player Data Changed")
	void K2_OnLobbyPlayerDataChanged(const TArray<int32>& PlayerData);

public:

	//~ Begin APawn Interface
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	//~ End APawn Interface
};
