// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Proxy/UpdateKronosSessionProxy.h"
#include "KronosOnlineSession.h"
#include "Kronos.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

UUpdateKronosSessionProxy* UUpdateKronosSessionProxy::UpdateKronosGameSession(UObject* WorldContextObject, const FKronosSessionSettings& SessionSettings, const TArray<FKronosSessionSetting>& ExtraSessionSettings, bool bShouldRefreshOnlineData)
{
	UUpdateKronosSessionProxy* Proxy = NewObject<UUpdateKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_GameSession;
	Proxy->SessionSettings = SessionSettings;
	Proxy->ExtraSessionSettings = ExtraSessionSettings;
	Proxy->bShouldRefreshOnlineData = bShouldRefreshOnlineData;
	return Proxy;
}

UUpdateKronosSessionProxy* UUpdateKronosSessionProxy::UpdateKronosPartySession(UObject* WorldContextObject, const FKronosSessionSettings& SessionSettings, const TArray<FKronosSessionSetting>& ExtraSessionSettings, bool bShouldRefreshOnlineData)
{
	UUpdateKronosSessionProxy* Proxy = NewObject<UUpdateKronosSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = NAME_PartySession;
	Proxy->SessionSettings = SessionSettings;
	Proxy->ExtraSessionSettings = ExtraSessionSettings;
	Proxy->bShouldRefreshOnlineData = bShouldRefreshOnlineData;
	return Proxy;
}

void UUpdateKronosSessionProxy::Activate()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			UKronosOnlineSession* KronosOnlineSession = UKronosOnlineSession::Get(this);
			if (KronosOnlineSession)
			{
				FOnUpdateSessionCompleteDelegate OnUpdateSessionCompleteDelegate = FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnUpdateSessionComplete);

				SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnUpdateSessionCompleteDelegateHandle);
				OnUpdateSessionCompleteDelegateHandle = SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(OnUpdateSessionCompleteDelegate);

				KronosOnlineSession->UpdateSession(SessionName, SessionSettings, bShouldRefreshOnlineData, ExtraSessionSettings);
				return;
			}

		}
	}

	OnUpdateSessionComplete(SessionName, false);
}

void UUpdateKronosSessionProxy::OnUpdateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("OnUpdateSessionComplete with result: %s"), (bWasSuccessful ? TEXT("Success") : TEXT("Failure")));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnUpdateSessionCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast();
		return;
	}

	OnFailure.Broadcast();
	return;
}
