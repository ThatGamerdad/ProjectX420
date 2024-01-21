// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/KronosReadFriendsListProxy.h"
#include "KronosUserManager.h"
#include "KronosTypes.h"
#include "Kronos.h"
#include "Interfaces/OnlineFriendsInterface.h"

UKronosReadFriendsListProxy* UKronosReadFriendsListProxy::ReadKronosFriendsList(UObject* WorldContextObject, bool bInGamePlayersOnly)
{
	UKronosReadFriendsListProxy* Proxy = NewObject<UKronosReadFriendsListProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->bInGamePlayersOnly = bInGamePlayersOnly;
	return Proxy;
}

void UKronosReadFriendsListProxy::Activate()
{
	FString FriendsListToRead = EFriendsLists::ToString(EFriendsLists::OnlinePlayers);
	if (bInGamePlayersOnly)
	{
		FriendsListToRead = EFriendsLists::ToString(EFriendsLists::InGamePlayers);
	}

	FOnReadFriendsListComplete CompletionDelegate = FOnReadFriendsListComplete::CreateUObject(this, &ThisClass::OnReadFriendsListComplete);

	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	UserManager->ReadFriendsList(FriendsListToRead, CompletionDelegate);
}

void UKronosReadFriendsListProxy::OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnReadFriendsListComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));
	UE_CLOG(!ErrorStr.IsEmpty(), LogKronos, Log, TEXT("ErrorStr: %s"), *ErrorStr);

	TArray<FKronosOnlineFriend> OutKronosFriends = TArray<FKronosOnlineFriend>();

	if (!bWasSuccessful)
	{
		OnFailure.Broadcast(OutKronosFriends);
		return;
	}

	UKronosUserManager* UserManager = UKronosUserManager::Get(WorldContextObject);
	if (!UserManager->GetFriendsList(ListName, OutKronosFriends))
	{
		OnFailure.Broadcast(OutKronosFriends);
		return;
	}

	UE_LOG(LogKronos, Log, TEXT("Friend count: %d"), OutKronosFriends.Num());
	OnSuccess.Broadcast(OutKronosFriends);
}
