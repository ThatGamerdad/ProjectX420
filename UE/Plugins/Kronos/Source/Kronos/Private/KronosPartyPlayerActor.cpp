// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosPartyPlayerActor.h"
#include "KronosPartyPlayerStart.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "Widgets/KronosPartyPlayerWidget.h"
#include "Kronos.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/ArrowComponent.h"

AKronosPartyPlayerActor::AKronosPartyPlayerActor()
{
    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
    CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
    CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CapsuleComponent->SetCollisionObjectType(ECC_Pawn);
    CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Block);
    CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
    RootComponent = CapsuleComponent;

    ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
    if (ArrowComponent)
    {
        ArrowComponent->ArrowColor = FColor(150, 200, 255);
        ArrowComponent->bIsScreenSizeScaled = true;
        ArrowComponent->SetupAttachment(RootComponent);
    }
}

void AKronosPartyPlayerActor::BeginPlay()
{
    Super::BeginPlay();

    // Hide the actor until initial replication is finished and the actor finds a player start.
    GetRootComponent()->SetVisibility(false, true);

    // Begin waiting initial replication.
    WaitInitialReplication();
}

void AKronosPartyPlayerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Before we are destroyed, clear our ownership of the player start so that other players can use it.
    if (OwnedPlayerStart)
    {
        OwnedPlayerStart->SetOwner(nullptr);
        OwnedPlayerStart = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AKronosPartyPlayerActor::WaitInitialReplication()
{
    // Check if initial replication finished.
    if (HasInitialReplicationFinished())
    {
        OnInitialReplicationFinished();
        return;
    }

    // Initial replication not finished. Checking again next frame...
    else
    {
        // Make sure that the actor is not pending kill.
        if (IsValid(this))
        {
            GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::WaitInitialReplication);
        }
    }
}

bool AKronosPartyPlayerActor::HasInitialReplicationFinished_Implementation() const
{
    AKronosPartyPlayerState* OwningPlayerState = GetOwningPartyPlayerState();
    return OwningPlayerState && OwningPlayerState->UniqueId.IsValid() && OwningPlayerState->PartyOwnerUniqueId.IsValid()
        && OwningPlayerState->GetPlayerName().IsEmpty() != true;
}

void AKronosPartyPlayerActor::OnInitialReplicationFinished()
{
    AKronosPartyPlayerState* OwningPlayerState = GetOwningPartyPlayerState();

    // Find the first available free (not owned) player start and spawn on it.
    AKronosPartyPlayerStart* FreePlayerStart = AKronosPartyPlayerStart::FindFreePlayerStart(this, OwningPlayerState->IsLocalPlayer());
    if (FreePlayerStart)
    {
        // Claim the player start.
        // Before we are destroyed, ownership will be cleared.
        FreePlayerStart->SetOwner(this);
        OwnedPlayerStart = FreePlayerStart;

        // Teleport the player to the player start.
        SetActorLocationAndRotation(FreePlayerStart->GetActorLocation(), FreePlayerStart->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
    }
    else
    {
        UE_LOG(LogKronos, Error, TEXT("KronosPartyPlayerActor: Could not find player start for %s (%s)."), *OwningPlayerState->UniqueId.ToDebugString(),
            OwningPlayerState->IsLocalPlayer() ? TEXT("Local") : TEXT("Remote"));
    }

    // Early out if there's no player widget class selected, or if we don't want to create for local player.
    if (!PlayerWidgetClass.Get() || (OwningPlayerState->IsLocalPlayer() && !bCreateForLocalPlayer))
    {
        OnPlayerActorInitialized();
        return;
    }

    // Create the widget renderer for the actor.
    CreateWidgetRenderer();
}

void AKronosPartyPlayerActor::CreateWidgetRenderer()
{
    // Make sure that only one widget renderer exists.
    if (WidgetComponent)
    {
        WidgetComponent->DestroyComponent();
    }

    // Create the widget component that renders the player widget.
    WidgetComponent = NewObject<UWidgetComponent>(this);
    WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    WidgetComponent->SetWidgetClass(PlayerWidgetClass);
    WidgetComponent->SetDrawSize(PlayerWidgetDrawSize);
    WidgetComponent->SetRelativeLocation(PlayerWidgetOffset);
    WidgetComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    WidgetComponent->RegisterComponent();

    // The widget component may not have the widget created yet, so we'll start awaiting it.
    WaitWidget();
}

void AKronosPartyPlayerActor::WaitWidget()
{
    UUserWidget* PlayerWidget = WidgetComponent ? WidgetComponent->GetUserWidgetObject() : nullptr;
    if (PlayerWidget)
    {
        // Initialize the party player widget.
        UKronosPartyPlayerWidget* PartyPlayerWidget = Cast<UKronosPartyPlayerWidget>(PlayerWidget);
        if (PartyPlayerWidget)
        {
            AKronosPartyPlayerState* OwningPlayerState = GetOwningPartyPlayerState();
            PartyPlayerWidget->InitPlayerWidget(OwningPlayerState);
        }

        // Signal that initialization has finished.
        OnPlayerActorInitialized();
        return;
    }

    // The player widget is not ready. Checking again next frame...
    else
    {
        // Make sure that the actor is not pending kill.
        if (IsValid(this))
        {
            GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::WaitWidget);
        }
    }
}

void AKronosPartyPlayerActor::OnPlayerActorInitialized()
{
    // Now that the actor is initialized we can unhide it.
    GetRootComponent()->SetVisibility(true, true);

    // Give blueprints a chance to implement custom logic.
    K2_OnPlayerActorInitialized();
}

AKronosPartyPlayerState* AKronosPartyPlayerActor::GetOwningPartyPlayerState() const
{
    return GetOwner<AKronosPartyPlayerState>();
}

UKronosPartyPlayerWidget* AKronosPartyPlayerActor::GetPlayerWidget() const
{
    return WidgetComponent ? Cast<UKronosPartyPlayerWidget>(WidgetComponent->GetUserWidgetObject()) : nullptr;
}
