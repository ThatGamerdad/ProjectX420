// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Widgets/KronosPartyPlayerWidget.h"
#include "Beacons/KronosPartyPlayerState.h"
#include "Kronos.h"

void UKronosPartyPlayerWidget::InitPlayerWidget(AKronosPartyPlayerState* InOwningPlayerState)
{
	if (InOwningPlayerState)
	{
		OwningPlayerState = InOwningPlayerState;

		OwningPlayerState->OnKronosPartyPlayerEloChanged().AddUObject(this, &ThisClass::OnPlayerEloChanged);
		OwningPlayerState->OnKronosPartyPlayerDataChanged().AddUObject(this, &ThisClass::OnPlayerDataChanged);
		OwningPlayerState->OnPartyOwnerChanged().AddUObject(this, &ThisClass::OnPartyLeaderChanged);

		K2_OnPlayerWidgetInitialized();

		// Make sure that we didn't miss a player data change.
		// Checking after widget initialization, so that during initialization we can init relevant UI elements, and wait for this delegate to trigger.
		TArray<int32> PlayerData = OwningPlayerState->GetPlayerData();
		if (PlayerData.Num() > 0)
		{
			OnPlayerDataChanged(PlayerData);
		}

		// Make sure that we didn't miss a party owner change.
		// Checking after widget initialization, so that during initialization we can just simply hide the party leader icon, and wait for this delegate to trigger.
		if (OwningPlayerState->PartyOwnerUniqueId.IsValid())
		{
			OnPartyLeaderChanged(OwningPlayerState->PartyOwnerUniqueId);
		}
	}
	
	else UE_LOG(LogKronos, Error, TEXT("KronosPartyPlayerWidget: Failed to initialize!"));
}

FUniqueNetIdRepl& UKronosPartyPlayerWidget::GetPlayerUniqueId() const
{
	if (OwningPlayerState)
	{
		return OwningPlayerState->UniqueId;
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPlayerWidget: GetPlayerUniqueId() called on player widget with no OwningPlayerState."));
	static FUniqueNetIdRepl EmptyId = FUniqueNetIdRepl();
	return EmptyId;
}

FText UKronosPartyPlayerWidget::GetPlayerName() const
{
	if (OwningPlayerState)
	{
		return OwningPlayerState->DisplayName;
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPlayerWidget: GetPlayerName() called on player widget with no OwningPlayerState."));
	return FText::GetEmpty();
}

int32 UKronosPartyPlayerWidget::GetPlayerElo() const
{
	if (OwningPlayerState)
	{
		return OwningPlayerState->GetPlayerElo();
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPlayerWidget: GetPlayerElo() called on player widget with no OwningPlayerState."));
	return 0;
}

TArray<int32> UKronosPartyPlayerWidget::GetPlayerData() const
{
	if (OwningPlayerState)
	{
		return OwningPlayerState->GetPlayerData();
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPlayerWidget: GetPlayerData() called on player widget with no OwningPlayerState."));
	return TArray<int32>();
}

bool UKronosPartyPlayerWidget::IsPartyLeader() const
{
	if (OwningPlayerState)
	{
		return OwningPlayerState->IsPartyLeader();
	}

	UE_LOG(LogKronos, Error, TEXT("KronosPlayerWidget: IsPartyLeader() called on player widget with no OwningPlayerState."));
	return false;
}

void UKronosPartyPlayerWidget::OnPlayerEloChanged(const int32& PlayerElo)
{
	K2_OnPlayerEloChanged(PlayerElo);
}

void UKronosPartyPlayerWidget::OnPlayerDataChanged(const TArray<int32>& PlayerData)
{
	K2_OnPlayerDataChanged(PlayerData);
}

void UKronosPartyPlayerWidget::OnPartyLeaderChanged(const FUniqueNetIdRepl& UniqueId)
{
	K2_OnPartyLeaderChanged();
}
