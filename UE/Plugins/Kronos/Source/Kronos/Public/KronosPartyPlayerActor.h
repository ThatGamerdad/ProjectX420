// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KronosPartyPlayerActor.generated.h"

class AKronosPartyPlayerState;
class AKronosPartyPlayerStart;
class UKronosPartyPlayerWidget;
class UCapsuleComponent;
class UArrowComponent;
class UWidgetComponent;

/**
 * KronosPartyPlayerActor is an actor that represents a player in the party.
 * Intended to be used for the actor class in the KronosPartyState.
 */
UCLASS(Blueprintable, Abstract, hideCategories = (Input, Replication))
class KRONOS_API AKronosPartyPlayerActor : public AActor
{
	GENERATED_BODY()
	
public:

	/** Default constructor. */
	AKronosPartyPlayerActor();

public:

	/**
	 * Optional widget to create for the player.
	 * This widget will be rendered to the screen automatically at the actor's location.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Party Actor")
	TSubclassOf<UKronosPartyPlayerWidget> PlayerWidgetClass = nullptr;

	/** Draw size of the player widget's 'canvas'. Does not scale the actual widget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Party Actor")
	FVector2D PlayerWidgetDrawSize = FVector2D(350.0f, 350.0f);

	/** Widget offset from the center of the actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Party Actor")
	FVector PlayerWidgetOffset = FVector::ZeroVector;

	/** Whether the local player should have a widget created or not. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Party Actor")
	bool bCreateForLocalPlayer = true;

protected:

	/** Collision component of the actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComponent;

	/** Arrow component visualizing the actors direction. */
	UPROPERTY()
	UArrowComponent* ArrowComponent;

	/** The component that renders the player's widget. */
	UPROPERTY(Transient)
	UWidgetComponent* WidgetComponent;

	/**
	 * The player start that this actor spawned on.
	 * Ownership of this player start will be cleared before the player actor is destroyed.
	 */
	UPROPERTY(Transient)
	AKronosPartyPlayerStart* OwnedPlayerStart;

public:

	/** Get the party player state of this player. */
	UFUNCTION(BlueprintPure, Category = "Default")
	AKronosPartyPlayerState* GetOwningPartyPlayerState() const;

	/** Get the party player start that this actor spawned on. */
	UFUNCTION(BlueprintPure, Category = "Default")
	AKronosPartyPlayerStart* GetOwnedPlayerStart() const { return OwnedPlayerStart; }

	/** Get the player actor's widget that is being rendered to the screen. */
	UFUNCTION(BlueprintPure, Category = "Default")
	UKronosPartyPlayerWidget* GetPlayerWidget() const;

	/** Get the widget component that renders the player widget to the screen. */
	UFUNCTION(BlueprintPure, Category = "Default")
	UWidgetComponent* GetWidgetComponent() const { return WidgetComponent; }

protected:

	/** Check if all necessary objects and values have been replicated. If not, the check will be performed again in the next frame. */
	virtual void WaitInitialReplication();

	/**
	 * Get whether initial replication has finished.
	 * Can be overridden to include additional replicated values that need to replicate before the actor is considered initialized.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Default")
	bool HasInitialReplicationFinished() const;

	/** Called after initial replication has finished. */
	virtual void OnInitialReplicationFinished();

	/** Create the widget component that renders the player's widget. This will also create the actual widget itself. */
	virtual void CreateWidgetRenderer();

	/** Check if the player widget has been created. If not, the check will be performed again in the next frame. */
	virtual void WaitWidget();

	/** Called when the player actor has been initialized. */
	virtual void OnPlayerActorInitialized();

protected:

	/** Event when the player actor has been initialized. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On Player Actor Initialized")
	void K2_OnPlayerActorInitialized();

public:

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface
};
