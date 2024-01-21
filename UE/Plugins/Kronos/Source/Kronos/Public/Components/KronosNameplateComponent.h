// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KronosNameplateComponent.generated.h"

class UUserWidget;
class UWidgetComponent;

/**
 * Delegate triggered when the nameplate is created.
 * 
 * @param NameplateWidget The widget that was created.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosNameplateCreated, UUserWidget*, NameplateWidget);

/**
 * Component that renders a widget above the player's head.
 * This component must be attached to a Pawn or Character.
 */
UCLASS(Blueprintable, classGroup = Kronos, meta = (BlueprintSpawnableComponent))
class KRONOS_API UKronosNameplateComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UKronosNameplateComponent(const FObjectInitializer& ObjectInitializer);

public:

	/**
	 * The widget to use for the nameplate.
	 * If the nameplate widget is a KronosLobbyPlayerWidget then it will be initialized automatically.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Nameplate")
	TSubclassOf<UUserWidget> NameplateWidgetClass;

	/** Draw size of the nameplate widget's 'canvas'. Does not scale the actual widget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Nameplate")
	FVector2D NameplateDrawSize;

	/** Nameplate offset from the center of the actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Nameplate")
	FVector NameplateOffset;

	/** Whether the local player should have a nameplate or not. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Nameplate")
	bool bCreateForLocalPlayer;

private:

	/** The component that renders the nameplate widget. */
	UPROPERTY(Transient)
	UWidgetComponent* WidgetComponent;

	/** Event when the nameplate is created. At this point the player state has replicated. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosNameplateCreated OnNameplateCreatedEvent;

public:

	/** Set the draw size of the nameplate widget's 'canvas'. Does not scale the actual widget. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetNameplateDrawSize(FVector2D InDrawSize);

	/** Set the offset of the nameplate from the center of the actor. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void SetNameplateOffset(FVector InOffset);

	/** Get whether a nameplate widget was created for the player. */
	UFUNCTION(BlueprintPure, Category = "Default")
	bool HasNameplate() const;

	/** Get the actual nameplate widget. */
	UFUNCTION(BlueprintPure, Category = "Default")
	UUserWidget* GetNameplateWidget() const;

	/** Get the component that renders the nameplate widget. */
	UFUNCTION(BlueprintPure, Category = "Default")
	UWidgetComponent* GetWidgetComponent() const { return WidgetComponent; }

	/** @return The delegate fired when the nameplate is created. */
	FOnKronosNameplateCreated& OnNameplateCreated() { return OnNameplateCreatedEvent; }

protected:

	/** Check if all necessary objects have been replicated. If not, the check will be performed again in the next frame. */
	virtual void WaitInitialReplication();

	/** Create the nameplate for the player. */
	virtual void CreateNameplate();

	/** Check if the nameplate widget has been created. If not, the check will be performed again in the next frame. */
	virtual void WaitForNameplateWidget();

public:

	//~ Begin UActorComponent Interface
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	//~ End UActorComponent Interface
};
