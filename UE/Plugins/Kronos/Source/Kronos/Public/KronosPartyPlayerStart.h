// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KronosPartyPlayerStart.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class UBillboardComponent;

/**
 * An actor used to mark a location where a party member could spawn on.
 */
UCLASS(classGroup = Kronos, hideCategories = (Components, Input, Physics, Collision, Navigation, Replication, Cooking, LOD, HLOD))
class KRONOS_API AKronosPartyPlayerStart : public AActor
{
	GENERATED_BODY()
	
public:

	/** Default constructor. */
	AKronosPartyPlayerStart(const FObjectInitializer& ObjectInitializer);

public:

	/**
	 * Whether the local player should spawn on this player start.
	 * One of the player starts should be marked this way.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party Player Start")
	bool bIsLocalPlayerStart;

protected:

	/** Collision component of the player start. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComponent;

public:

	/**
	 * Find the first available player start that is free (has no owner).
	 * Called by the KronosPartyPlayerActor after its initial replication has finished.
	 */
	static AKronosPartyPlayerStart* FindFreePlayerStart(const UObject* WorldContextObject, bool bFindLocal);

	/** Check whether this player start is free, meaning no player spawned on it yet. */
	UFUNCTION(BlueprintPure, Category = "Default")
	bool IsFree() const { return GetOwner() == nullptr; }

	/** @return Capsule component of the player start. */
	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

#if WITH_EDITORONLY_DATA
protected:

	/** Arrow component visualizing spawn direction. */
	UPROPERTY()
	UArrowComponent* ArrowComponent;

	/** Sprite component visible in editor. */
	UPROPERTY()
	UBillboardComponent* SpriteComponent;

public:

	/** @return Arrow component of the player start. */
	UArrowComponent* GetArrowComponent() const { return ArrowComponent; }

	/** @return Sprite component of the player start. */
	UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }

#endif // WITH_EDITORONLY_DATA
};
