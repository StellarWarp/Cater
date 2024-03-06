// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineKeyValuePair.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystem.h"
#include "Containers/Ticker.h"
#include "Input/Reply.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "CaterGameInstance.generated.h"


class UCaterUIManager;
class UCaterMessageMenuWidget;
class UCaterInGameMenuWidget;
class UCaterMainMenuWidget;
class ACaterGameSession;

namespace CaterGameInstanceState
{
	extern const FName None;
	extern const FName PendingInvite;
	extern const FName MainMenu;
	extern const FName MessageMenu;
	extern const FName Playing;
}

UENUM()
enum class EOnlineMode : uint8
{
	Offline,
	LAN,
	Online
};

UCLASS()
class UCaterGameInstance : public UGameInstance
{
private:
	GENERATED_BODY()

	UCaterGameInstance();
	FTickerDelegate TickDelegate;

	bool Tick(float DeltaSeconds);

public:
	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void StartGameInstance() override;

#if WITH_EDITOR
	FGameInstancePIEResult StartPlayInEditorGameInstance(ULocalPlayer* LocalPlayer,
	                                                     const FGameInstancePIEParameters& Params) override;
#endif

	/// UI

private:
	UPROPERTY()
	UCaterUIManager* UIManager;

	UPROPERTY(config, EditDefaultsOnly)
	TSubclassOf<UCaterUIManager> UIManagerClass;

public:

	UCaterUIManager* GetUIManager() const { return UIManager; }
	/// State Management

private:
	FName CurrentState;
	FName PendingState;

public:
	const FName GetCurrentState() const;

	void BeginNewState(FName Name, FName OldState);
	void EndCurrentState(FName Name);

	void GotoState(FName NewState);
	void TickState();
	FName GetInitialState();
	void GotoInitialState();
	bool LoadFrontEndMap(const FString& MapName);
	void TravelLocalSessionFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ReasonString);
	void SetPresenceForLocalPlayers(const FString& StatusStr, const FVariantData& PresenceData);

	/** Returns true if the game is in online mode */
	EOnlineMode GetOnlineMode() const { return OnlineMode; }

	// void BeginPendingInviteState();
	void BeginMainMenuState();
	void BeginMessageMenuState();
	void BeginPlayingState();

	// void EndPendingInviteState();
	void EndMainMenuState();
	void EndMessageMenuState();
	void EndPlayingState();

	//blueprint callable function to get the current state
	UFUNCTION(BlueprintCallable, Category = "UCaterGameInstance")
	void InitMainMenu();

	/// Online 
private:
	UPROPERTY(config, EditDefaultsOnly)
	FString WelcomeScreenMap;

	UPROPERTY(config, EditDefaultsOnly)
	FString MainMenuMap;

	/** URL to travel to after pending network operations */
	FString TravelURL;

	/** Current online mode of the game (offline, LAN, or online) */
	EOnlineMode OnlineMode;

public:
	ACaterGameSession* GetGameSession() const;

	void SetOnlineMode(EOnlineMode InOnlineMode);
	void TravelToSession(const FName& SessionName);
	void SetIgnorePairingChangeForControllerId(int32 ControllerId);
	bool IsLocalPlayerOnline(ULocalPlayer* LocalPlayer);
	bool IsLocalPlayerSignedIn(ULocalPlayer* LocalPlayer);
	bool ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer);
	bool ValidatePlayerIsSignedIn(ULocalPlayer* LocalPlayer);
	FReply OnConfirmGeneric();
	void OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);
	void FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result);


	bool HostGame(ULocalPlayer* LocalPlayer, const FString& GameType, const FString& InTravelURL);
	bool FindSessions(ULocalPlayer* PlayerOwner, bool bIsDedicatedServer, bool bFindLAN);
	void OnSearchSessionsComplete(bool bWasSuccessful);
	bool JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults);
	bool JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult);
	void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);
	void FinishJoinSession(EOnJoinSessionCompleteResult::Type Result);
	void OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result);
	void InternalTravelToSession(const FName& SessionName);
	void OnCreatePresenceSessionComplete(FName SessionName, bool bWasSuccessful);
	// void SetPendingInvite(const FCaterPendingInvite& InPendingInvite);

	FString GetQuickMatchUrl();
	void BeginHostingQuickMatch();

	bool HostQuickSession(ULocalPlayer& LocalPlayer, const FOnlineSessionSettings& SessionSettings);

private:
	/** Controller to ignore for pairing changes. -1 to skip ignore. */
	int32 IgnorePairingChangeForControllerId;

	/** Last connection status that was passed into the HandleNetworkConnectionStatusChanged hander */
	EOnlineServerConnectionStatus::Type CurrentConnectionStatus;


	/** Handle to various registered delegates */
	FDelegateHandle TickDelegateHandle;
	FDelegateHandle TravelLocalSessionFailureDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FDelegateHandle OnSearchSessionsCompleteDelegateHandle;
	FDelegateHandle OnStartSessionCompleteDelegateHandle;
	FDelegateHandle OnEndSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	FDelegateHandle OnCreatePresenceSessionCompleteDelegateHandle;

	/** Local player login status when the system is suspended */
	TArray<ELoginStatus::Type> LocalPlayerOnlineStatus;

private:
	void HandleUserLoginChanged(int I, ELoginStatus::Type Arg, ELoginStatus::Type Arg1,
	                            const FUniqueNetId& UniqueNetId);
	void HandleControllerPairingChanged(int GameUserIndex, FControllerPairingChangedUserInfo PreviousUserInfo,
	                                    FControllerPairingChangedUserInfo
	                                    NewUserInfo);
	void HandleAppWillDeactivate();
	void HandleAppSuspend();
	void HandleAppResume();
	void HandleSafeFrameChanged();
	void HandleControllerConnectionChange(bool bArg, int I, int Arg);
	void OnPreLoadMap(const FString& String);
	void OnPostLoadMap(UWorld* World);
	void OnPostDemoPlay();

	void UpdateUsingMultiplayerFeatures(bool bIsUsingMultiplayerFeatures);


	void RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer);
	FReply OnPairingUsePreviousProfile();
	FReply OnPairingUseNewProfile();
	FReply OnControllerReconnectConfirm();
	TSharedPtr<const FUniqueNetId> GetUniqueNetIdFromControllerId(const int ControllerId);


	void HandleNetworkConnectionStatusChanged(const FString& String, EOnlineServerConnectionStatus::Type Arg,
	                                          EOnlineServerConnectionStatus::Type Arg1);
	void HandleSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type Arg);

	/** Delegate for ending a session */
	FOnEndSessionCompleteDelegate OnEndSessionCompleteDelegate;
	void OnEndSessionComplete(FName Name, bool bArg);
	void CleanupSessionOnReturnToMenu();
	void LabelPlayerAsQuitter(ULocalPlayer* LocalPlayer) const;
	void RemoveNetworkFailureHandlers();
	void AddNetworkFailureHandlers();


	bool HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld);
	bool HandleDisconnectCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld);
	bool HandleTravelCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld);
	void HandleSignInChangeMessaging();
};
