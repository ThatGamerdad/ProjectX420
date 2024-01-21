// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Lobby/KronosLobbyPawn.h"
#include "Lobby/KronosLobbyGameState.h"
#include "Lobby/KronosLobbyPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/PlayerStart.h"

AKronosLobbyPawn::AKronosLobbyPawn(const FObjectInitializer& ObjectInitializer)
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
		ArrowComponent->SetupAttachment(CapsuleComponent);
	}

	SetReplicateMovement(false);
}

void AKronosLobbyPawn::BeginPlay()
{
	Super::BeginPlay();

	// Handles the initial relocation of the pawn (in case we want the local player to be in a fixed spot).
	InitPawnLocation();
}

void AKronosLobbyPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server side binding of the lobby player data changed event.

	AKronosLobbyPlayerState* LobbyPlayerState = GetPlayerState<AKronosLobbyPlayerState>();
	if (LobbyPlayerState)
	{
		LobbyPlayerState->OnLobbyPlayerDataChanged.AddDynamic(this, &ThisClass::OnLobbyPlayerDataChanged);

		// Make sure that we didn't miss a player data change event.
		TArray<int32> PlayerData = LobbyPlayerState->GetPlayerData();
		if (PlayerData.Num() > 0)
		{
			OnLobbyPlayerDataChanged(PlayerData);
		}
	}
}

void AKronosLobbyPawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client side binding of the lobby player data changed event.

	AKronosLobbyPlayerState* LobbyPlayerState = GetPlayerState<AKronosLobbyPlayerState>();
	if (LobbyPlayerState)
	{
		LobbyPlayerState->OnLobbyPlayerDataChanged.AddDynamic(this, &ThisClass::OnLobbyPlayerDataChanged);

		// Make sure that we didn't miss a player data change event.
		TArray<int32> PlayerData = LobbyPlayerState->GetPlayerData();
		if (PlayerData.Num() > 0)
		{
			OnLobbyPlayerDataChanged(PlayerData);
		}
	}
}

void AKronosLobbyPawn::InitPawnLocation()
{
	AKronosLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AKronosLobbyGameState>();
	if (LobbyGameState)
	{
		if (LobbyGameState->bRelocatePlayers)
		{
			// Finding a proper player start for the given player. Since we want our local player to be in a fixed spot, we'll relocate players locally.
			// Lobby pawns have movement replication disabled, so moving them won't cause any issues over the network.

			APlayerStart* PlayerStart = LobbyGameState->FindPlayerStart(this);
			if (PlayerStart)
			{
				// Make sure to tell the player start that we are taking it.
				PlayerStart->SetOwner(this);

				// Move the player.
				SetActorLocationAndRotation(PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation());
			}
		}
	}
}

void AKronosLobbyPawn::OnLobbyPlayerDataChanged(const TArray<int32>& PlayerData)
{
	K2_OnLobbyPlayerDataChanged(PlayerData);
}
