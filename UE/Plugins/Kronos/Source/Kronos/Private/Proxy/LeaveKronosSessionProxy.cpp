// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/LeaveKronosSessionProxy.h"
#include "KronosOnlineSession.h"
#include "KronosPartyManager.h"
#include "Kronos.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"

ULeaveKronosSessionProxy* ULeaveKronosSessionProxy::LeaveKronosGameSession(UObject* WorldContextObject)
{
	ULeaveKronosSessionProxy* Proxy = NewObject<ULeaveKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_GameSession;
	return Proxy;
}

ULeaveKronosSessionProxy* ULeaveKronosSessionProxy::LeaveKronosPartySession(UObject* WorldContextObject)
{
	ULeaveKronosSessionProxy* Proxy = NewObject<ULeaveKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_PartySession;
	return Proxy;
}

void ULeaveKronosSessionProxy::Activate()
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (SessionName == NAME_PartySession)
		{
			UKronosPartyManager* PartyManager = UKronosPartyManager::Get(WorldContextObject);
			PartyManager->LeaveParty(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnLeavePartyComplete));
			return;
		}

		if (SessionName == NAME_GameSession)
		{
			// Player is hosting the match. Gracefully tell all clients then the host to return to the main menu.
			if (World->GetNetMode() < NM_Client)
			{
				AGameModeBase* GameMode = World->GetAuthGameMode();
				if (GameMode)
				{
					AGameSession* GameSession = GameMode->GameSession;
					if (GameSession)
					{
						GameSession->ReturnToMainMenuHost();
						OnComplete.Broadcast();
						return;
					}
				}
			}

			// Player is a client in the match. Simply return to the main menu.
			else
			{
				World->GetGameInstance()->ReturnToMainMenu();
				OnComplete.Broadcast();
				return;
			}
		}
	}
}

void ULeaveKronosSessionProxy::OnLeavePartyComplete(FName InSessionName, bool bWasSuccessful)
{
	OnComplete.Broadcast();
}
