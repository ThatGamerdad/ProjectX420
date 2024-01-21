// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Widgets/KronosPartyWidget.h"
#include "Widgets/KronosPartyPlayerWidget.h"
#include "KronosPartyManager.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "KronosStatics.h"
#include "Components/DynamicEntryBox.h"
#include "Engine/GameInstance.h"

void UKronosPartyWidget::NativeOnInitialized()
{
	UKronosPartyManager* PartyManager = UKronosPartyManager::Get(this);
	PartyManager->OnConnectedToParty().AddDynamic(this, &ThisClass::OnConnectedToParty);
	PartyManager->OnDisconnectedFromParty().AddDynamic(this, &ThisClass::OnDisconnectedFromParty);
	PartyManager->OnPlayerJoinedParty().AddDynamic(this, &ThisClass::OnPlayerJoinedParty);
	PartyManager->OnPlayerLeftParty().AddDynamic(this, &ThisClass::OnPlayerLeftParty);
	PartyManager->OnPlayerStateAdded().AddDynamic(this, &ThisClass::OnPlayerStateAdded);
	PartyManager->OnPlayerStateRemoved().AddDynamic(this, &ThisClass::OnPlayerStateRemoved);
	PartyManager->OnChatMessageReceived().AddDynamic(this, &ThisClass::OnChatMessageReceived);

	// Create player widgets for existing party players.
	if (PartyManager->IsInParty())
	{
		for (AKronosPartyPlayerState* PartyPlayer : PartyManager->GetPartyPlayerStates())
		{
			CreatePlayerWidget(PartyPlayer);
		}
	}

	Super::NativeOnInitialized();
}

void UKronosPartyWidget::CreatePlayerWidget(AKronosPartyPlayerState* OwningPlayerState)
{
	if (PartyPlayerEntryBox)
	{
		bool bIsLocalPlayer = GetGameInstance()->GetPrimaryPlayerUniqueIdRepl() == OwningPlayerState->UniqueId;
		if (bIsLocalPlayer ? bCreateEntryForLocalPlayer : true)
		{
			UKronosPartyPlayerWidget* PlayerWidget = PartyPlayerEntryBox->CreateEntry<UKronosPartyPlayerWidget>();
			if (PlayerWidget)
			{
				PlayerWidget->InitPlayerWidget(OwningPlayerState);
			}
		}

		RecreatePartySlotWidgets();
	}
}

void UKronosPartyWidget::RemovePlayerWidget(const FUniqueNetIdRepl& PlayerId)
{
	if (PartyPlayerEntryBox)
	{
		for (UKronosPartyPlayerWidget* PlayerWidget : PartyPlayerEntryBox->GetTypedEntries<UKronosPartyPlayerWidget>())
		{
			if (PlayerWidget->GetPlayerUniqueId() == PlayerId)
			{
				PartyPlayerEntryBox->RemoveEntry(PlayerWidget);

				RecreatePartySlotWidgets();
				return;
			}
		}
	}
}

void UKronosPartyWidget::RecreatePartySlotWidgets()
{
	if (PartySlotEntryBox && PartyPlayerEntryBox)
	{
		PartySlotEntryBox->Reset(true);

		FKronosSessionSettings SessionSettings;
		if (UKronosStatics::GetPartySessionSettings(this, SessionSettings))
		{
			int32 NumWidgetsToCreate = SessionSettings.MaxNumPlayers - PartyPlayerEntryBox->GetNumEntries();
			if (!bCreateEntryForLocalPlayer)
			{
				NumWidgetsToCreate--;
			}

			if (NumWidgetsToCreate > 0)
			{
				for (int32 i = 0; i < NumWidgetsToCreate; i++)
				{
					PartySlotEntryBox->CreateEntry();
				}
			}
		}
	}
}

void UKronosPartyWidget::OnConnectedToParty()
{
	K2_OnConnectedToParty();
}

void UKronosPartyWidget::OnDisconnectedFromParty()
{
	if (PartyPlayerEntryBox) PartyPlayerEntryBox->Reset(true);
	if (PartySlotEntryBox) PartySlotEntryBox->Reset(true);

	K2_OnDisconnectedFromParty();
}

void UKronosPartyWidget::OnPlayerStateAdded(AKronosPartyPlayerState* PlayerState)
{
	CreatePlayerWidget(PlayerState);
}

void UKronosPartyWidget::OnPlayerStateRemoved(AKronosPartyPlayerState* PlayerState)
{
	RemovePlayerWidget(PlayerState->UniqueId);
}

void UKronosPartyWidget::OnPlayerJoinedParty(const FText& DisplayName, const FUniqueNetIdRepl& PlayerId)
{
	K2_OnPlayerJoinedParty(DisplayName, PlayerId);
}

void UKronosPartyWidget::OnPlayerLeftParty(const FUniqueNetIdRepl& PlayerId)
{
	K2_OnPlayerLeftParty(PlayerId);
}

void UKronosPartyWidget::OnChatMessageReceived(const FUniqueNetIdRepl& SenderId, const FString& Msg)
{
	K2_OnChatMessageReceived(SenderId, Msg);
}
