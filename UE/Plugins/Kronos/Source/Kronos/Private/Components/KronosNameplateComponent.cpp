// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Components/KronosNameplateComponent.h"
#include "Lobby/KronosLobbyPlayerState.h"
#include "Widgets/KronosLobbyPlayerWidget.h"
#include "Kronos.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

UKronosNameplateComponent::UKronosNameplateComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NameplateWidgetClass = TSubclassOf<UUserWidget>();
	NameplateDrawSize = FVector2D(150.0f, 50.0f);
	NameplateOffset = FVector(0.0f, 0.0f, 100.0f);
	bCreateForLocalPlayer = true;

	bWantsInitializeComponent = true;
}

void UKronosNameplateComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (!GetOwner()->IsA<APawn>())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosNameplateComponent: Owning actor is invalid! The component must be attached to a Pawn or Character."));
		return;
	}
	
	if (!NameplateWidgetClass.Get())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosNameplateComponent: NameplateWidgetClass is empty!"));
		return;
	}

	// Start waiting for initial replication to finish.
	WaitInitialReplication();
}

void UKronosNameplateComponent::WaitInitialReplication()
{
	APawn* PlayerPawn = GetOwner<APawn>();
	APlayerState* PlayerState = PlayerPawn ? PlayerPawn->GetPlayerState() : nullptr;

	// Check if initial object replication finished or not.
	if (PlayerState)
	{
		// If this pawn is the local player, check if we want to create the nameplate.
		if (PlayerPawn->IsLocallyControlled() && !bCreateForLocalPlayer)
		{
			return;
		}

		// Create the nameplate for the player.
		CreateNameplate();
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

void UKronosNameplateComponent::CreateNameplate()
{
	// Make sure that only one nameplate renderer exists.
	if (WidgetComponent)
	{
		WidgetComponent->DestroyComponent();
	}

	// Create the widget component that renders the nameplate.
	WidgetComponent = NewObject<UWidgetComponent>(GetOwner());
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetWidgetClass(NameplateWidgetClass);
	WidgetComponent->SetDrawSize(NameplateDrawSize);
	WidgetComponent->SetRelativeLocation(NameplateOffset);
	WidgetComponent->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	WidgetComponent->RegisterComponent();

	// The widget component may not have the nameplate widget created yet, so we'll start awaiting it.
	WaitForNameplateWidget();
}

void UKronosNameplateComponent::WaitForNameplateWidget()
{
	// Check if the nameplate widget has been created.
	UUserWidget* NameplateWidget = WidgetComponent ? WidgetComponent->GetUserWidgetObject() : nullptr;
	if (NameplateWidget)
	{
		// If the nameplate widget is a lobby player widget, automatically initialize it.
		if (NameplateWidget->IsA<UKronosLobbyPlayerWidget>())
		{
			AKronosLobbyPlayerState* LobbyPlayerState = Cast<AKronosLobbyPlayerState>(GetOwner<APawn>()->GetPlayerState());
			if (LobbyPlayerState)
			{
				UKronosLobbyPlayerWidget* LobbyPlayerWidget = Cast<UKronosLobbyPlayerWidget>(WidgetComponent->GetUserWidgetObject());
				LobbyPlayerWidget->InitPlayerWidget(LobbyPlayerState);
			}
		}

		// Notification that the nameplate was created.
		OnNameplateCreatedEvent.Broadcast(NameplateWidget);
	}

	// The nameplate widget is not ready. Checking again in the next frame...
	else
	{
		// Make sure that the owner is not pending kill.
		if (IsValid(this) && IsValid(GetOwner()))
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::WaitForNameplateWidget);
		}
	}
}

void UKronosNameplateComponent::SetNameplateDrawSize(FVector2D InDrawSize)
{
	NameplateDrawSize = InDrawSize;
	if (WidgetComponent)
	{
		WidgetComponent->SetDrawSize(InDrawSize);
	}
}

void UKronosNameplateComponent::SetNameplateOffset(FVector InOffset)
{
	NameplateOffset = InOffset;
	if (WidgetComponent)
	{
		WidgetComponent->SetRelativeLocation(InOffset);
	}
}

bool UKronosNameplateComponent::HasNameplate() const
{
	return GetNameplateWidget() != nullptr;
}

UUserWidget* UKronosNameplateComponent::GetNameplateWidget() const
{
	return WidgetComponent ? WidgetComponent->GetUserWidgetObject() : nullptr;
}

void UKronosNameplateComponent::UninitializeComponent()
{
	if (WidgetComponent)
	{
		WidgetComponent->DestroyComponent();
	}

	Super::UninitializeComponent();
}
