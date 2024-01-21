// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "KronosUserManager.generated.h"

class UKronosUserAuthWidget;
class AGameModeBase;
struct FKronosOnlineFriend;
using FTimerDelegate = TDelegate<void(), FNotThreadSafeNotCheckedDelegateUserPolicy>;

/**
 * Delegate triggered when user authentication is started.
 * 
 * @param bIsInitialAuth Whether it's the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKronosUserAuthStarted, bool, bIsInitialAuth);

/**
 * Delegate triggered when user authentication state is changed.
 * 
 * @param NewState New authentication state.
 * @param PrevState Previous authentication state.
 * @param bIsInitialAuth Whether it's the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnKronosUserAuthStateChanged, EKronosUserAuthState, NewState, EKronosUserAuthState, PrevState, bool, bIsInitialAuth);

/**
 * Delegate triggered when user authentication is complete.
 *
 * @param Result Result of authentication. Anything other than Success is a form of failure and user should be prompted to restart authentication.
 * @param bWasInitialAuth Whether this was the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
 * @param ErrorText Detailed description of the error. Can be displayed to the user.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnKronosUserAuthComplete, EKronosUserAuthCompleteResult, Result, bool, bWasInitialAuth, const FText&, ErrorText);

/**
 * KronosUserManager handles the authentication of the local user, and implements online user related functionality such as online identity and friends.
 * It is automatically spawned and managed by the KronosOnlineSession class.
 * 
 * User authentication is started automatically when the game default map is loaded (unless configured otherwise).
 * Authentication should be called every time the user enters the game default map (e.g. entering main menu for first time, or returning to main menu from a match).
 * During authentication the system will make sure that the user is logged in with the Online Subsystem properly.
 * While authentication is in progress the user should not be able to interact with the game.
 * By default a widget will be displayed showing the state of authentication.
 */
UCLASS(Blueprintable, BlueprintType)
class KRONOS_API UKronosUserManager : public UObject
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UKronosUserManager(const FObjectInitializer& ObjectInitializer);

public:

	/**
	 * Widget to display when user authentication is started. Can be left empty if no widget is needed.
	 * This widget is NOT removed automatically.
	 * The lifecycle of this widget can be managed by the game itself.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "User Manager")
	TSubclassOf<UKronosUserAuthWidget> AuthWidgetClass;

	/**
	 * The widget being displayed during user authentication.
	 * Public accessible so that games can manage its lifecycle.
	 */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "User Manager")
	UKronosUserAuthWidget* AuthWidget;

	/**
	 * If enabled, user authentication will fail immediately.
	 * This is useful when you want to test if the game handles authentication failure properly.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug")
	bool bDebugSimulateAuthFailure;

protected:

	/**
	 * Whether the user is currently authenticated.
	 * Updated during auth requests (e.g. entering main menu for first time, or returning to main menu from a match).
	 */
	bool bIsAuthenticated;

	/** Is user authentication in progress. */
	bool bAuthInProgress;

	/** Is user logout in progress. */
	bool bLogoutInProgress;

	/** Current state of user authentication (while in-progress). */
	EKronosUserAuthState CurrentAuthState;

	/**
	 * Time when the last auth task was started.
	 * Used to delay the next auth task if the current task finished earlier that the configured min time.
	 */
	float LastAuthTaskStartTime;

	/** Handle for platform login complete delegate. */
	FDelegateHandle PlatformLoginDelegateHandle;

	/** Handle for platform logout complete delegate. */
	FDelegateHandle PlatformLogoutDelegateHandle;

	/** Handle used when the user auth flow doesn't want to go into the next state immediately. */
	FTimerHandle TimerHandle_ChangeAuthState;

private:

	/** Event when user authentication is started. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosUserAuthStarted OnUserAuthStartedEvent;

	/** Event when user authentication state is changed. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosUserAuthStateChanged OnUserAuthStateChangedEvent;

	/** Event when user authentication is complete. */
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess))
	FOnKronosUserAuthComplete OnUserAuthCompleteEvent;

public:

	/** Get the user manager from the KronosOnlineSession. */
	UFUNCTION(BlueprintPure, Category = "Kronos", DisplayName = "Get Kronos User Manager", meta = (WorldContext = "WorldContextObject"))
	static UKronosUserManager* Get(const UObject* WorldContextObject);

	/**
	 * Initialize the KronosUserManager during game startup.
	 * Called by the KronosOnlineSession.
	 */
	virtual void Initialize() {};

	/**
	 * Deinitialize the KronosUserManager before game shutdown.
	 * Called by the KronosOnlineSession.
	 */
	virtual void Deinitialize() {};

public:

	/**
	 * Begin authenticating the user.
	 * Ensures that the user is logged in with the online platform.
	 * The plugin calls this automatically unless configured otherwise.
	 * Should be called every time the user enters the game default map (e.g. entering main menu for first time, or returning to main menu from a match).
	 * If this returns false, no completion event will be called!
	 * 
	 * @return Whether authentication is started or not. If false, no completion event will be called!
	 */
	UFUNCTION(BlueprintCallable, Category = "Default")
	bool AuthenticateUser();

	/**
	 * Whether the user is currently authenticated.
	 * Updated during auth requests (e.g. entering main menu for first time, or returning to main menu from a match).
	 */
	UFUNCTION(BlueprintPure, Category = "Default")
	bool IsAuthenticated() const { return bIsAuthenticated; }

	/** Whether user authentication is in progress. */
	UFUNCTION(BlueprintPure, Category = "Default")
	bool IsAuthenticatingUser() const { return bAuthInProgress; }

	/** Get the current state of user authentication (while in-progress). */
	UFUNCTION(BlueprintPure, Category = "Default")
	EKronosUserAuthState GetCurrentAuthState() const { return CurrentAuthState; }

	/** Whether game logout flow is in progress or not. */
	UFUNCTION(BlueprintPure, Category = "Default")
	bool IsLogoutInProgress() const { return bLogoutInProgress; }

	/**
	 * Whether the local user is currently logged in with the online subsystem or not.
	 * This is only the current online status of the user!
	 * User authentication may still be in progress even if this returns true.
	 */
	bool IsLoggedIn() const;

	/** @return The local user's unique id. */
	FUniqueNetIdPtr GetUserId() const;

	/** @return The local user's nickname. */
	FString GetUserNickname() const;

	/** @return The delegate fired when user authentication is started. */
	FOnKronosUserAuthStarted& OnKronosUserAuthStarted() { return OnUserAuthStartedEvent; }

	/** @return The delegate fired when user authentication state is changed. */
	FOnKronosUserAuthStateChanged& OnKronosUserAuthStateChanged() { return OnUserAuthStateChangedEvent; }

	/** @return The delegate fired when user authentication is complete. */
	FOnKronosUserAuthComplete& OnKronosUserAuthComplete() { return OnUserAuthCompleteEvent; }

protected:

	/** Called when user authentication is started. */
	virtual void OnUserAuthStarted(bool bIsInitialAuth);

	/** Called when user authentication state is changed. */
	virtual void OnUserAuthStateChanged(EKronosUserAuthState NewState, EKronosUserAuthState PrevState, bool bIsInitialAuth);

	/**
	 * Entry point for user authentication.
	 * Ensures that the user is logged in with the online subsystem.
	 */
	virtual void PlatformLogin();

	/**
	 * Get the credentials to use when logging in with the online subsystem.
	 * Called during platform login of user authentication.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Default")
	void GetLoginCredentials(FString& LoginType, FString& LoginId, FString& LoginToken);

	/** Called when login with online subsystem is complete. */
	virtual void OnPlatformLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& ErrorStr);

	/**
	 * Begin reading user files from the cloud.
	 * Called after platform login.
	 */
	virtual void ReadUserFiles(const FUniqueNetId& UserId);

	/** Called when reading user files is complete. */
	virtual void OnReadUserFilesComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& ErrorStr);

	/**
	 * Begin any custom authentication tasks. Called after reading user files from cloud.
	 * You can override this function to implement any additional tasks for your game (e.g. read values from your custom backend service).
	 * Once you are done with everything, make sure to end by calling OnCustomAuthTasksComplete.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Default")
	void BeginCustomAuthTasks();

	/**
	 * Signals that custom auth tasks have finished.
	 * Should be called by the user to signal that authentication can finish.
	 * Only needed when implementing custom auth tasks!
	 */
	UFUNCTION(BlueprintCallable, Category = "Default")
	void OnCustomAuthTasksComplete(bool bWasSuccessful, const FText& ErrorText);

	/**
	 * Called when user authentication is complete.
	 * Anything other than Success is a form of failure and the user should be prompted to authenticate again.
	 */
	virtual void OnUserAuthComplete(EKronosUserAuthCompleteResult Result, bool bWasInitialAuth, const FText& ErrorText);

	/** Changes authentication state. */
	void ChangeAuthState(EKronosUserAuthState NewState);

	/**
	 * Helper function to start the next auth task while adhering to the configured min task time.
	 * The given delegate will be executed latently if the current auth task finished earlier then the configured min task time.
	 */
	void BeginAuthTaskLatent(const FTimerDelegate& NextAuthTaskDelegate);

public:

	/**
	 * Begin logging out the user.
	 * Ensures that the user is logged out from the online platform.
	 * Most games are not going to use this, but it's here as an option if you want users to be able to login with different accounts.
	 * The user will have to be logged back in using the AuthenticateUser function.
	 * If this returns false, no completion event will be called!
	 * 
	 * @return Whether user logout is started or not. If false, no completion event will be called!
	 */
	UFUNCTION(BlueprintCallable, Category = "Default")
	bool LogoutUser();

protected:

	/** Called when user logout is started. */
	virtual void OnUserLogoutStarted();

	/**
	 * Entry point for user logout.
	 * Ensures that the user is logged out from the online subsystem.
	 */
	virtual void PlatformLogout();

	/** Called when logout from the online subsystem is complete. */
	virtual void OnPlatformLogoutComplete(int32 LocalUserNum, bool bWasSuccessful);

	/** Called when user logout is complete. */
	virtual void OnUserLogoutComplete(bool bWasSuccessful);

public:

	/**
	 * Read the given friends list of the user through the online subsystem.
	 * For possible list names see EFriendsLists.
	 */
	virtual bool ReadFriendsList(const FString& ListName, const FOnReadFriendsListComplete& CompletionDelegate = FOnReadFriendsListComplete());

	/**
	 * Get the given cached friends list.
	 * Only valid after reading friends list!
	 * For possible list names see EFriendsLists.
	 */
	virtual bool GetFriendsList(const FString& ListName, TArray<FKronosOnlineFriend>& OutFriends);

	/**
	 * Get a specific friend from the given cached friends list.
	 * Only valid after reading friends list!
	 * For possible list names see EFriendsLists.
	 */
	virtual FKronosOnlineFriend GetFriend(const FUniqueNetId& FriendId, const FString& ListName);

	/**
	 * Get the number of friends contained in the given cached friends list.
	 * Only valid after reading friends list!
	 * For possible list names see EFriendsLists.
	 */
	virtual int32 GetFriendCount(const FString& ListName) const;

	/**
	 * Get whether the user is friends with the given player.
	 * An additional list name can be provided for the online subsystem for further processing.
	 * Steam doesn't make use of this list, while EOS can use "onlinePlayers" and "inGamePlayers" names to filter friends.
	 * For possible list names see EFriendsLists.
	 */
	virtual bool IsFriend(const FUniqueNetId& FriendId, const FString& ListName = TEXT("")) const;

	/**
	 * Send a session invite to the given friend.
	 * Session name must be either NAME_GameSession or NAME_PartySession!
	 */
	virtual bool SendSessionInviteToFriend(FName InSessionName, const FUniqueNetId& FriendId);

protected:

	/**
	 * Event when user authentication is started.
	 *
	 * @param bIsInitialAuth Whether it's the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On User Auth Started")
	void K2_OnUserAuthStarted(bool bIsInitialAuth);

	/**
	 * Event when user authentication state is changed.
	 *
	 * @param NewState New authentication state.
	 * @param PrevState Previous authentication state.
	 * @param bIsInitialAuth Whether it's the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On User Auth State Changed")
	void K2_OnUserAuthStateChanged(EKronosUserAuthState NewState, EKronosUserAuthState PrevState, bool bIsInitialAuth);

	/**
	 * Event when user authentication is complete.
	 *
	 * @param Result Authentication result. Anything other than Success is a form of failure and the user should be prompted to authenticate again.
	 * @param bWasInitialAuth Whether this was the first auth of the user. False if the user was authenticated already and we are just confirming that his login status is not lost.
	 * @param ErrorText Detailed description of the error. Can be displayed to the user.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On User Auth Complete")
	void K2_OnUserAuthComplete(EKronosUserAuthCompleteResult Result, bool bWasInitialAuth, const FText& ErrorText);

	/** Event when user logout is started. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On User Logout Started")
	void K2_OnUserLogoutStarted();

	/**
	 * Event when user logout is complete.
	 * 
	 * @param bWasSuccessful Whether user logout was successful or not.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events", DisplayName = "On User Logout Complete")
	void K2_OnUserLogoutComplete(bool bWasSuccessful);

public:

	//~ Begin UObject interface
	virtual UWorld* GetWorld() const final; // Required by blueprints to have access to gameplay statics and other world context based nodes.
	//~ End UObject interface
};
