// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosUserManager.h"
#include "KronosOnlineSession.h"
#include "Widgets/KronosUserAuthWidget.h"
#include "Kronos.h"
#include "KronosConfig.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"

#define LOCTEXT_NAMESPACE "Kronos"
static const FText AuthError_IdentityInterfaceInvalid = LOCTEXT("AuthError_IdentityInterfaceInvalid", "The IdentityInterface of the Online Subsystem was invalid.");
static const FText AuthError_SimulatedAuthFailure = LOCTEXT("AuthError_SimulatedAuthFailure", "Simulated auth failure for testing purposes. You can disable this in the KronosUserManager class.");
static const FText AuthError_PlatformLoginFailed = LOCTEXT("AuthError_PlatformLoginFailed", "Could not connect to the online platform. Please check your internet connection and try again.");
static const FText AuthError_LoginStatusLost = LOCTEXT("AuthError_LoginStatusLost", "You have lost connection with the online platform. Please check your internet connection and try again.");
static const FText AuthError_ReadUserFilesFailed = LOCTEXT("AuthError_ReadUserFilesFailed", "Could not read user files from the cloud. Please check your internet connection and try again.");
#undef LOCTEXT_NAMESPACE

UKronosUserManager::UKronosUserManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	/*
	 * NOTE: This caused blueprint compile errors when packaging the projects.
	 * As a quick hotfix I'll just disable this for v2.0.2.
	 * We'll need to find a proper solution to this though.
	 */

// 	static ConstructorHelpers::FClassFinder<UKronosUserAuthWidget> DefaultAuthWidgetClass(TEXT("/Kronos/Internal/Widgets/WBP_DefaultKronosAuthWidget.WBP_DefaultKronosAuthWidget_C"));
// 	if (DefaultAuthWidgetClass.Succeeded())
// 	{
// 		AuthWidgetClass = DefaultAuthWidgetClass.Class;
// 	}
}

UKronosUserManager* UKronosUserManager::Get(const UObject* WorldContextObject)
{
	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(WorldContextObject);
	if (OnlineSession)
	{
		return OnlineSession->GetUserManager();
	}

	return nullptr;
}

bool UKronosUserManager::AuthenticateUser()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosUserManager: Authenticating user..."));

	if (bAuthInProgress)
	{
		UE_LOG(LogKronos, Warning, TEXT("Authentication already in progress."));
		return false;
	}

	if (bLogoutInProgress)
	{
		UE_LOG(LogKronos, Warning, TEXT("Can't authenticate while logout is in progress."));
		return false;
	}

	bAuthInProgress = true;

	// Create the user auth widget.
	if (AuthWidgetClass != nullptr)
	{
		APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController();
		if (IsValid(LocalPlayerController))
		{
			// Make sure that only one auth widget exists.
			if (IsValid(AuthWidget))
			{
				AuthWidget->RemoveFromParent();
			}

			AuthWidget = CreateWidget<UKronosUserAuthWidget>(LocalPlayerController, AuthWidgetClass);
			AuthWidget->AddToViewport();
		}
	}

	// Notification that user authentication is starting.
	OnUserAuthStarted(!bIsAuthenticated);

	LastAuthTaskStartTime = GetWorld()->GetTimeSeconds();
	ChangeAuthState(EKronosUserAuthState::PlatformLogin);

#if !UE_BUILD_SHIPPING
	// Simulate auth failure.
	// Only if we are in non shipping builds.
	if (bDebugSimulateAuthFailure)
	{
		OnUserAuthComplete(EKronosUserAuthCompleteResult::UnknownError, !bIsAuthenticated, AuthError_SimulatedAuthFailure);
		return true;
	}
#endif

	// User auth flow is: PlatformLogin -> ReadUserFiles -> CustomAuthTasks (implemented by end user of plugin) -> Auth complete.
	PlatformLogin();
	return true;
}

void UKronosUserManager::OnUserAuthStarted(bool bIsInitialAuth)
{
	K2_OnUserAuthStarted(bIsInitialAuth);
	OnUserAuthStartedEvent.Broadcast(bIsInitialAuth);
}

void UKronosUserManager::OnUserAuthStateChanged(EKronosUserAuthState NewState, EKronosUserAuthState PrevState, bool bIsInitialAuth)
{
	K2_OnUserAuthStateChanged(NewState, PrevState, bIsInitialAuth);
	OnUserAuthStateChangedEvent.Broadcast(NewState, PrevState, bIsInitialAuth);
}

void UKronosUserManager::PlatformLogin()
{
	UE_LOG(LogKronos, Verbose, TEXT("Logging in to online platform..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!IdentityInterface.IsValid())
	{
		OnUserAuthComplete(EKronosUserAuthCompleteResult::PlatformLoginFailed, !bIsAuthenticated, AuthError_IdentityInterfaceInvalid);
		return;
	}

	ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(0);
	if (bIsAuthenticated && LoginStatus == ELoginStatus::NotLoggedIn)
	{
		OnUserAuthComplete(EKronosUserAuthCompleteResult::PlatformLoginStatusLost, false, AuthError_LoginStatusLost);
		return;
	}

	if (bIsAuthenticated && LoginStatus == ELoginStatus::LoggedIn)
	{
		OnPlatformLoginComplete(0, true, *IdentityInterface->GetUniquePlayerId(0), TEXT(""));
		return;
	}

	FOnLoginCompleteDelegate LoginCompleteDelegate = FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::OnPlatformLoginComplete);
	PlatformLoginDelegateHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(0, LoginCompleteDelegate);

	// Get the login credentials.
	FString LoginType;
	FString LoginId;
	FString LoginToken;
	GetLoginCredentials(LoginType, LoginId, LoginToken);

	if (OnlineSubsystem->GetSubsystemName() == NULL_SUBSYSTEM)
	{
		LoginId = GetUserNickname();
		UE_LOG(LogKronos, Log, TEXT("Login for Null Online Subsystem detected. Login id changed to '%s'"), *LoginId);
	}

	const FOnlineAccountCredentials AccountCredentials = FOnlineAccountCredentials(LoginType, LoginId, LoginToken);
	IdentityInterface->Login(0, AccountCredentials);
	return;
}

void UKronosUserManager::GetLoginCredentials_Implementation(FString& LoginType, FString& LoginId, FString& LoginToken)
{
	LoginType = TEXT("");
	LoginId = TEXT("");
	LoginToken = TEXT("");

	// For EOS we'll use the account portal login type by default.
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem && OnlineSubsystem->GetSubsystemName() == EOS_SUBSYSTEM)
	{
		LoginType = TEXT("AccountPortal");
	}
}

void UKronosUserManager::OnPlatformLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& ErrorStr)
{
	UE_LOG(LogKronos, Verbose, TEXT("OnPlatformLoginComplete with result: %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failure"));
	UE_CLOG(!ErrorStr.IsEmpty(), LogKronos, Verbose, TEXT("ErrorStr: %s"), *ErrorStr);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!IdentityInterface.IsValid())
	{
		OnUserAuthComplete(EKronosUserAuthCompleteResult::PlatformLoginFailed, !bIsAuthenticated, AuthError_IdentityInterfaceInvalid);
		return;
	}

	if (PlatformLoginDelegateHandle.IsValid())
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(0, PlatformLoginDelegateHandle);
		PlatformLoginDelegateHandle.Reset();
	}

	if (!bWasSuccessful)
	{
		OnUserAuthComplete(EKronosUserAuthCompleteResult::PlatformLoginFailed, !bIsAuthenticated, AuthError_PlatformLoginFailed);
		return;
	}

	// Go to the next auth state.
	// Will be delayed if the current auth task finished earlier then the configured min task time.
	// This is so that the user can see what is happening properly.
	BeginAuthTaskLatent(FTimerDelegate::CreateLambda([this]()
	{
		LastAuthTaskStartTime = GetWorld()->GetTimeSeconds();
		ChangeAuthState(EKronosUserAuthState::ReadUserFiles);

		const FUniqueNetId& UserId = *GetUserId();
		ReadUserFiles(UserId);
	}));
}

void UKronosUserManager::ReadUserFiles(const FUniqueNetId& UserId)
{
	UE_LOG(LogKronos, Verbose, TEXT("Reading user files from cloud..."));

	// Not implemented for now.
	// If you override this function make sure to call OnReadUserFilesComplete!

	UE_LOG(LogKronos, Verbose, TEXT("ReadUserFiles not implemented, skipping..."));
	OnReadUserFilesComplete(true, UserId, TEXT(""));
}

void UKronosUserManager::OnReadUserFilesComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& ErrorStr)
{
	UE_LOG(LogKronos, Verbose, TEXT("OnReadUserFilesComplete with result: %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failure"));
	UE_CLOG(!ErrorStr.IsEmpty(), LogKronos, Verbose, TEXT("ErrorStr: %s"), *ErrorStr);

	if (!bWasSuccessful)
	{
		OnUserAuthComplete(EKronosUserAuthCompleteResult::ReadUserFilesFailed, !bIsAuthenticated, AuthError_ReadUserFilesFailed);
		return;
	}

	// Go to the next auth state.
	// Will be delayed if the current auth task finished earlier then the configured min task time.
	// This is so that the user can see what is happening properly.
	BeginAuthTaskLatent(FTimerDelegate::CreateLambda([this]()
	{
		LastAuthTaskStartTime = GetWorld()->GetTimeSeconds();
		ChangeAuthState(EKronosUserAuthState::CustomAuthTask);
		BeginCustomAuthTasks();
	}));
}

void UKronosUserManager::BeginCustomAuthTasks_Implementation()
{
	// Override this function to implement custom logic.
	// Make sure to call OnCustomAuthTasksComplete once you are done!
	OnCustomAuthTasksComplete(true, FText::GetEmpty());
}

void UKronosUserManager::OnCustomAuthTasksComplete(bool bWasSuccessful, const FText& ErrorText)
{
	if (!bWasSuccessful)
	{
		OnUserAuthComplete(EKronosUserAuthCompleteResult::CustomAuthTaskFailed, !bIsAuthenticated, ErrorText);
		return;
	}

	OnUserAuthComplete(EKronosUserAuthCompleteResult::Success, !bIsAuthenticated, FText::GetEmpty());
}

void UKronosUserManager::OnUserAuthComplete(EKronosUserAuthCompleteResult Result, bool bWasInitialAuth, const FText& ErrorText)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosUserManager: OnUserAuthComplete with result: %s"), LexToString(Result));

	bAuthInProgress = false;
	ChangeAuthState(EKronosUserAuthState::NotAuthenticating);

	if (Result == EKronosUserAuthCompleteResult::Success)
	{
		// User authentication was successful.
		// The user is now authenticated.
		bIsAuthenticated = true;
	}
	else
	{
		// User authentication failed.
		// Make sure to clear the auth state of the user.
		bIsAuthenticated = false;
	}

	// Give blueprints a chance to implement custom logic.
	K2_OnUserAuthComplete(Result, bWasInitialAuth, ErrorText);

	// Notify the online session that user auth is complete.
	UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(this);
	OnlineSession->HandleUserAuthComplete(Result, bWasInitialAuth, ErrorText);

	// Signal auth complete.
	OnUserAuthCompleteEvent.Broadcast(Result, bWasInitialAuth, ErrorText);
}

void UKronosUserManager::ChangeAuthState(EKronosUserAuthState NewState)
{
	EKronosUserAuthState PrevAuthState = CurrentAuthState;
	CurrentAuthState = NewState;

	OnUserAuthStateChanged(CurrentAuthState, PrevAuthState, !bIsAuthenticated);
}

void UKronosUserManager::BeginAuthTaskLatent(const FTimerDelegate& NextAuthTaskDelegate)
{
	const float WorldTime = GetWorld()->GetTimeSeconds();
	const float MinTimeBeforeNextTask = LastAuthTaskStartTime + GetDefault<UKronosConfig>()->MinTimePerAuthTask;
	if (WorldTime < MinTimeBeforeNextTask)
	{
		const float TimeDelta = MinTimeBeforeNextTask - WorldTime;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ChangeAuthState, NextAuthTaskDelegate, TimeDelta, false);
	}
	else
	{
		NextAuthTaskDelegate.Execute();
	}
}

bool UKronosUserManager::LogoutUser()
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosUserManager: Logging out..."));

	if (bLogoutInProgress)
	{
		UE_LOG(LogKronos, Warning, TEXT("Logout already in progress."));
		return false;
	}

	if (bAuthInProgress)
	{
		UE_LOG(LogKronos, Warning, TEXT("Can't logout while user authentication is in progress."));
		return false;
	}

	bLogoutInProgress = true;
	OnUserLogoutStarted();

	PlatformLogout();
	return true;
}

void UKronosUserManager::OnUserLogoutStarted()
{
	K2_OnUserLogoutStarted();
}

void UKronosUserManager::PlatformLogout()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!IdentityInterface.IsValid())
	{
		OnUserLogoutComplete(false);
		return;
	}

	FOnLogoutCompleteDelegate LogoutCompleteDelegate = FOnLogoutCompleteDelegate::CreateUObject(this, &ThisClass::OnPlatformLogoutComplete);
	PlatformLogoutDelegateHandle = IdentityInterface->AddOnLogoutCompleteDelegate_Handle(0, LogoutCompleteDelegate);

	IdentityInterface->Logout(0);
}

void UKronosUserManager::OnPlatformLogoutComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!IdentityInterface.IsValid())
	{
		OnUserLogoutComplete(false);
		return;
	}

	IdentityInterface->ClearOnLogoutCompleteDelegate_Handle(0, PlatformLogoutDelegateHandle);
	PlatformLogoutDelegateHandle.Reset();

	OnUserLogoutComplete(true);
}

void UKronosUserManager::OnUserLogoutComplete(bool bWasSuccessful)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosUserManager: OnUserLogoutComplete with result: %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failure"));

	bLogoutInProgress = false;
	bIsAuthenticated = false;

	K2_OnUserLogoutComplete(bWasSuccessful);
}

bool UKronosUserManager::IsLoggedIn() const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!IdentityInterface.IsValid())
	{
		return false;
	}

	return IdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

FUniqueNetIdPtr UKronosUserManager::GetUserId() const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!IdentityInterface.IsValid())
	{
		return FUniqueNetIdPtr(nullptr);
	}

	return IdentityInterface->GetUniquePlayerId(0);
}

FString UKronosUserManager::GetUserNickname() const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!IdentityInterface.IsValid())
	{
		return TEXT("");
	}

	FString Nickname = IdentityInterface->GetPlayerNickname(0);
	return Nickname.Left(20);
}

bool UKronosUserManager::ReadFriendsList(const FString& ListName, const FOnReadFriendsListComplete& CompletionDelegate)
{
	KRONOS_LOG(LogKronos, Log, COLOR_DARK_CYAN, TEXT("KronosUserManager: Reading friends list..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogKronos, Warning, TEXT("FriendsInterface invalid (current Online Subsystem may not support it)."));
		CompletionDelegate.ExecuteIfBound(0, false, ListName, TEXT("FriendsInterface invalid."));
		return false;
	}

	return FriendsInterface->ReadFriendsList(0, ListName, CompletionDelegate);
}

bool UKronosUserManager::GetFriendsList(const FString& ListName, TArray<FKronosOnlineFriend>& OutFriends)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosUserManager: GetFriendsList failed - FriendsInterface invalid (current Online Subsystem may not support it)."));
		return false;
	}

	TArray<TSharedRef<FOnlineFriend>> OnlineFriends = TArray<TSharedRef<FOnlineFriend>>();
	if (!FriendsInterface->GetFriendsList(0, ListName, OnlineFriends))
	{
		UE_LOG(LogKronos, Error, TEXT("KronosUserManager: GetFriendsList failed - Could not retrieve friends list from Online Subsystem."));
		return false;
	}

	for (const TSharedRef<FOnlineFriend>& OnlineFriend : OnlineFriends)
	{
		OutFriends.Emplace(*OnlineFriend);
	}

	return true;
}

FKronosOnlineFriend UKronosUserManager::GetFriend(const FUniqueNetId& FriendId, const FString& ListName)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosUserManager: GetFriend failed - FriendsInterface invalid (current Online Subsystem may not support it)."));
		return FKronosOnlineFriend();
	}

	if (!FriendId.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosUserManager: GetFriend failed - FriendId is invalid."));
		return FKronosOnlineFriend();
	}

	TSharedPtr<FOnlineFriend> OnlineFriend = FriendsInterface->GetFriend(0, FriendId, ListName);
	if (!OnlineFriend.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosUserManager: GetFriend failed - Could not find friend in cached friends list."));
		return FKronosOnlineFriend();
	}

	return FKronosOnlineFriend(*OnlineFriend);
}

int32 UKronosUserManager::GetFriendCount(const FString& ListName) const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosUserManager: GetFriendCount failed - FriendsInterface invalid (current Online Subsystem may not support it)."));
		return 0;
	}

	TArray<TSharedRef<FOnlineFriend>> OnlineFriends = TArray<TSharedRef<FOnlineFriend>>();
	FriendsInterface->GetFriendsList(0, ListName, OnlineFriends);

	return OnlineFriends.Num();
}

bool UKronosUserManager::IsFriend(const FUniqueNetId& FriendId, const FString& ListName) const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosUserManager: IsFriend failed - FriendsInterface invalid (current Online Subsystem may not support it)."));
		return false;
	}

	if (!FriendId.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosUserManager: IsFriend failed - FriendId is invalid."));
		return false;
	}

	return FriendsInterface->IsFriend(0, FriendId, ListName);
}

bool UKronosUserManager::SendSessionInviteToFriend(FName InSessionName, const FUniqueNetId& FriendId)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogKronos, Warning, TEXT("KronosUserManager: SendSessionInviteToFriend failed - SessionInterface invalid (current Online Subsystem may not support it)."));
		return false;
	}

	if (!FriendId.IsValid())
	{
		UE_LOG(LogKronos, Error, TEXT("KronosUserManager: SendSessionInviteToFriend failed - FriendId is invalid."));
		return false;
	}

	return SessionInterface->SendSessionInviteToFriend(0, InSessionName, FriendId);
}

UWorld* UKronosUserManager::GetWorld() const
{
	// We don't care about the CDO because it's not relevant to world checks.
	// If we don't return nullptr here then blueprints are going to complain.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	return GetOuter()->GetWorld();
}