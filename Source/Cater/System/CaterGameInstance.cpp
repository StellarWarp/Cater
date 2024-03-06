// Fill out your copyright notice in the Description page of Project Settings.


#include "CaterGameInstance.h"

#include "CaterCharacter.h"
#include "CaterGameState.h"
#include "CaterInGameMenuWidget.h"
#include "CaterMainMenuWidget.h"
#include "CaterMessageMenuWidget.h"
#include "CaterPlayerController.h"
#include "Containers/Ticker.h"
#include "Misc/CoreDelegates.h"
#include "OnlineSubsystemUtils.h"
#include "CaterUIManager.h"
#include "Engine/Canvas.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Online/CaterGameSession.h"


class ACaterGameState;

namespace CaterGameInstanceState
{
	const FName None = FName(TEXT("None"));
	const FName PendingInvite = FName(TEXT("PendingInvite"));
	const FName MainMenu = FName(TEXT("MainMenu"));
	const FName MessageMenu = FName(TEXT("MessageMenu"));
	const FName Playing = FName(TEXT("Playing"));
}

UCaterGameInstance::UCaterGameInstance(): OnlineMode(EOnlineMode::Online) // Default to online
{
	CurrentState = CaterGameInstanceState::None;
}

bool UCaterGameInstance::Tick(float DeltaSeconds)
{
	TickState();
	return true;
}


void UCaterGameInstance::Init()
{
	Super::Init();

	IgnorePairingChangeForControllerId = -1;
	CurrentConnectionStatus = EOnlineServerConnectionStatus::Connected;

	LocalPlayerOnlineStatus.InsertDefaulted(0, MAX_LOCAL_PLAYERS);

	// game requires the ability to ID users.
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);
	const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
	check(IdentityInterface.IsValid());

	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	check(SessionInterface.IsValid());

	// bind any OSS delegates we needs to handle
	for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
	{
		IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(
			i, FOnLoginStatusChangedDelegate::CreateUObject(this, &UCaterGameInstance::HandleUserLoginChanged));
	}

	IdentityInterface->AddOnControllerPairingChangedDelegate_Handle(
		FOnControllerPairingChangedDelegate::CreateUObject(this, &UCaterGameInstance::HandleControllerPairingChanged));

	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &UCaterGameInstance::HandleAppWillDeactivate);

	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UCaterGameInstance::HandleAppSuspend);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UCaterGameInstance::HandleAppResume);

	FCoreDelegates::OnSafeFrameChangedEvent.AddUObject(this, &UCaterGameInstance::HandleSafeFrameChanged);
	FCoreDelegates::OnControllerConnectionChange.
		AddUObject(this, &UCaterGameInstance::HandleControllerConnectionChange);

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UCaterGameInstance::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCaterGameInstance::OnPostLoadMap);

	FCoreUObjectDelegates::PostDemoPlay.AddUObject(this, &UCaterGameInstance::OnPostDemoPlay);


	OnlineSub->AddOnConnectionStatusChangedDelegate_Handle(
		FOnConnectionStatusChangedDelegate::CreateUObject(
			this, &UCaterGameInstance::HandleNetworkConnectionStatusChanged));

	if (SessionInterface.IsValid())
	{
		SessionInterface->AddOnSessionFailureDelegate_Handle(
			FOnSessionFailureDelegate::CreateUObject(this, &UCaterGameInstance::HandleSessionFailure));
	}

	OnEndSessionCompleteDelegate = FOnEndSessionCompleteDelegate::CreateUObject(
		this, &UCaterGameInstance::OnEndSessionComplete);

	// Register delegate for ticker callback
	TickDelegate = FTickerDelegate::CreateUObject(this, &UCaterGameInstance::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);

	//UI
	UIManager = NewObject<UCaterUIManager>(this, UIManagerClass);
	UIManager->Construct(this);
}

void UCaterGameInstance::Shutdown()
{
	Super::Shutdown();
	FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
	UIManager = nullptr;
}

void UCaterGameInstance::StartGameInstance()
{
	Super::StartGameInstance();

	GotoState(CaterGameInstanceState::MainMenu);
}
#if WITH_EDITOR

FGameInstancePIEResult UCaterGameInstance::StartPlayInEditorGameInstance(
	ULocalPlayer* LocalPlayer, const FGameInstancePIEParameters& Params)
{
	FWorldContext* PlayWorldContext = GetWorldContext();
	check(PlayWorldContext);

	UWorld* PlayWorld = PlayWorldContext->World();
	check(PlayWorld);

	// UGameplayStatics::OpenLevel(PlayWorld, TEXT("MainMenu"), true, TEXT("listen"));

	GotoState(CaterGameInstanceState::MainMenu);


	// FString CurrentMapName = PlayWorld->GetOutermost()->GetName();
	// if (!PlayWorldContext->PIEPrefix.IsEmpty())
	// {
	// 	CurrentMapName.ReplaceInline(*PlayWorldContext->PIEPrefix, TEXT(""));
	// }
	//
	// if (CurrentMapName == MainMenuMap || CurrentMapName == FString(TEXT("None")))
	// {
	// 	GotoState(CaterGameInstanceState::MainMenu);
	// }
	// else
	// {
	// 	GotoState(CaterGameInstanceState::Playing);
	// }

	return Super::StartPlayInEditorGameInstance(LocalPlayer, Params);
}
#endif	// WITH_EDITOR
/// UI


/// State Management

const FName UCaterGameInstance::GetCurrentState() const
{
	return CurrentState;
}

void UCaterGameInstance::BeginNewState(FName NewState, FName PrevState)
{
	// per-state custom starting code here

	// if (NewState == CaterGameInstanceState::PendingInvite)
	// {
	// 	BeginPendingInviteState();
	// }
	if (NewState == CaterGameInstanceState::MainMenu)
	{
		BeginMainMenuState();
	}
	else if (NewState == CaterGameInstanceState::MessageMenu)
	{
		BeginMessageMenuState();
	}
	else if (NewState == CaterGameInstanceState::Playing)
	{
		BeginPlayingState();
	}

	CurrentState = NewState;
}

void UCaterGameInstance::EndCurrentState(FName NextState)
{
	// per-state custom ending code here
	// if (CurrentState == CaterGameInstanceState::PendingInvite)
	// {
	// 	EndPendingInviteState();
	// }
	if (CurrentState == CaterGameInstanceState::MainMenu)
	{
		EndMainMenuState();
	}
	else if (CurrentState == CaterGameInstanceState::MessageMenu)
	{
		EndMessageMenuState();
	}
	else if (CurrentState == CaterGameInstanceState::Playing)
	{
		EndPlayingState();
	}

	CurrentState = CaterGameInstanceState::None;
}

void UCaterGameInstance::GotoState(FName NewState)
{
	UE_LOG(LogOnline, Log, TEXT( "GotoState: NewState: %s" ), *NewState.ToString());

	PendingState = NewState;
}

void UCaterGameInstance::TickState()
{
	if ((PendingState != CurrentState) && (PendingState != CaterGameInstanceState::None))
	{
		FName const OldState = CurrentState;

		// end current state
		EndCurrentState(PendingState);

		// begin new state
		BeginNewState(PendingState, OldState);

		// clear pending change
		PendingState = CaterGameInstanceState::None;
	}
}


FName UCaterGameInstance::GetInitialState()
{
	return CaterGameInstanceState::MainMenu;
}

void UCaterGameInstance::GotoInitialState()
{
	GotoState(GetInitialState());
}

// void UCaterGameInstance::ShowMessageThenGotoState( const FText& Message, const FText& OKButtonString, const FText& CancelButtonString, const FName& NewState, const bool OverrideExisting, TWeakObjectPtr< ULocalPlayer > PlayerOwner )
// {
// 	UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Message: %s, NewState: %s" ), *Message.ToString(), *NewState.ToString() );
//
// 	const bool bAtWelcomeScreen = PendingState == CaterGameInstanceState::WelcomeScreen || CurrentState == CaterGameInstanceState::WelcomeScreen;
//
// 	// Never override the welcome screen
// 	if ( bAtWelcomeScreen )
// 	{
// 		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Ignoring due to higher message priority in queue (at welcome screen)." ) );
// 		return;
// 	}
//
// 	const bool bAlreadyAtMessageMenu = PendingState == CaterGameInstanceState::MessageMenu || CurrentState == CaterGameInstanceState::MessageMenu;
// 	const bool bAlreadyAtDestState = PendingState == NewState || CurrentState == NewState;
//
// 	// If we are already going to the message menu, don't override unless asked to
// 	if ( bAlreadyAtMessageMenu && PendingMessage.NextState == NewState && !OverrideExisting )
// 	{
// 		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Ignoring due to higher message priority in queue (check 1)." ) );
// 		return;
// 	}
//
// 	// If we are already going to the message menu, and the next dest is welcome screen, don't override
// 	if ( bAlreadyAtMessageMenu && PendingMessage.NextState == CaterGameInstanceState::WelcomeScreen )
// 	{
// 		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Ignoring due to higher message priority in queue (check 2)." ) );
// 		return;
// 	}
//
// 	// If we are already at the dest state, don't override unless asked
// 	if ( bAlreadyAtDestState && !OverrideExisting )
// 	{
// 		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Ignoring due to higher message priority in queue (check 3)" ) );
// 		return;
// 	}
//
// 	PendingMessage.DisplayString		= Message;
// 	PendingMessage.OKButtonString		= OKButtonString;
// 	PendingMessage.CancelButtonString	= CancelButtonString;
// 	PendingMessage.NextState			= NewState;
// 	PendingMessage.PlayerOwner			= PlayerOwner;
//
// 	if ( CurrentState == CaterGameInstanceState::MessageMenu )
// 	{
// 		UE_LOG( LogOnline, Log, TEXT( "ShowMessageThenGotoState: Forcing new message" ) );
// 		EndMessageMenuState();
// 		BeginMessageMenuState();
// 	}
// 	else
// 	{
// 		GotoState(CaterGameInstanceState::MessageMenu);
// 	}
// }

// void UCaterGameInstance::ShowLoadingScreen()
// {
// 	// This can be confusing, so here is what is happening:
// 	//	For LoadMap, we use the ICaterGameLoadingScreenModule interface to show the load screen
// 	//  This is necessary since this is a blocking call, and our viewport loading screen won't get updated.
// 	//  We can't use ICaterGameLoadingScreenModule for seamless travel though
// 	//  In this case, we just add a widget to the viewport, and have it update on the main thread
// 	//  To simplify things, we just do both, and you can't tell, one will cover the other if they both show at the same time
// 	ICaterGameLoadingScreenModule* const LoadingScreenModule = FModuleManager::LoadModulePtr<ICaterGameLoadingScreenModule>("CaterGameLoadingScreen");
// 	if (LoadingScreenModule != nullptr)
// 	{
// 		LoadingScreenModule->StartInGameLoadingScreen();
// 	}
//
// 	UCaterGameViewportClient * CaterViewport = Cast<UCaterGameViewportClient>(GetGameViewportClient());
//
// 	if ( CaterViewport != NULL )
// 	{
// 		CaterViewport->ShowLoadingScreen();
// 	}
// }

bool UCaterGameInstance::LoadFrontEndMap(const FString& MapName)
{
	bool bSuccess = true;

	// if already loaded, do nothing
	UWorld* const World = GetWorld();
	if (World)
	{
		FString const CurrentMapName = *World->PersistentLevel->GetOutermost()->GetName();
		//if (MapName.Find(TEXT("Highrise")) != -1)
		if (CurrentMapName == MapName)
		{
			return bSuccess;
		}
	}

	FString Error;
	EBrowseReturnVal::Type BrowseRet = EBrowseReturnVal::Failure;
	FURL URL(
		*FString::Printf(TEXT("%s"), *MapName)
	);

	if (URL.Valid && !HasAnyFlags(RF_ClassDefaultObject))
	//CastChecked<UEngine>() will fail if using Default__CaterGameInstance, so make sure that we're not default
	{
		BrowseRet = GetEngine()->Browse(*WorldContext, URL, Error);

		// Handle failure.
		if (BrowseRet != EBrowseReturnVal::Success)
		{
			UE_LOG(LogLoad, Fatal, TEXT("%s"),
			       *FString::Printf(TEXT("Failed to enter %s: %s. Please check the log for errors."), *MapName, *Error
			       ));
			bSuccess = false;
		}
	}
	return bSuccess;
}

void UCaterGameInstance::TravelLocalSessionFailure(UWorld* World, ETravelFailure::Type FailureType,
                                                   const FString& ReasonString)
{
	// ACaterPlayerController_Menu* const FirstPC = Cast<ACaterPlayerController_Menu>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	// if (FirstPC != nullptr)
	// {
	// 	FText ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Join Session failed.");
	// 	if (ReasonString.IsEmpty() == false)
	// 	{
	// 		ReturnReason = FText::Format(NSLOCTEXT("NetworkErrors", "JoinSessionFailedReasonFmt", "Join Session failed. {0}"), FText::FromString(ReasonString));
	// 	}
	//
	// 	FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
	// 	ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
	// }
}

// void UCaterGameInstance::ShowMessageThenGoMain(const FText& Message, const FText& OKButtonString, const FText& CancelButtonString)
// {
// 	ShowMessageThenGotoState(Message, OKButtonString, CancelButtonString, CaterGameInstanceState::MainMenu);
// }

// void UCaterGameInstance::SetPendingInvite(const FCaterPendingInvite& InPendingInvite)
// {
// 	CaterGameInstanceState::PendingInvite = InPendingInvite;
// }


// void UCaterGameInstance::BeginPendingInviteState()
// {	
// 	if (LoadFrontEndMap(MainMenuMap))
// 	{				
// 		StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &UCaterGameInstance::OnUserCanPlayInvite), EUserPrivileges::CanPlayOnline, PendingInvite.UserId);
// 	}
// 	else
// 	{
// 		GotoState(CaterGameInstanceState::WelcomeScreen);
// 	}
// }
//
// void UCaterGameInstance::EndPendingInviteState()
// {
// 	// cleanup in case the state changed before the pending invite was handled.
// 	CleanupOnlinePrivilegeTask();
// }


void UCaterGameInstance::SetPresenceForLocalPlayers(const FString& StatusStr, const FVariantData& PresenceData)
{
	const IOnlinePresencePtr Presence = Online::GetPresenceInterface(GetWorld());
	if (Presence.IsValid())
	{
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			FUniqueNetIdRepl UserId = LocalPlayers[i]->GetPreferredUniqueNetId();

			if (UserId.IsValid())
			{
				FOnlineUserPresenceStatus PresenceStatus;
				PresenceStatus.StatusStr = StatusStr;
				PresenceStatus.State = EOnlinePresenceState::Online;
				PresenceStatus.Properties.Add(DefaultPresenceKey, PresenceData);

				Presence->SetPresence(*UserId, PresenceStatus);
			}
		}
	}
}

void UCaterGameInstance::BeginMainMenuState()
{
	// SetOnlineMode(EOnlineMode::Offline);
	SetOnlineMode(EOnlineMode::Online);


	// Disallow splitscreen
	// UGameViewportClient* GameViewportClient = GetGameViewportClient();
	//
	// if (GameViewportClient)
	// {
	// 	GetGameViewportClient()->SetForceDisableSplitscreen(true);
	// }
	//
	//
	// // Set presence to menu state for the owning player
	// SetPresenceForLocalPlayers(FString(TEXT("In Menu")), FVariantData(FString(TEXT("OnMenu"))));
	//
	// // load startup map
	// LoadFrontEndMap(MainMenuMap);

	// player 0 gets to own the UI
	ULocalPlayer* const Player = GetFirstGamePlayer();

	UIManager->SetUIState(ECaterUIState::MainMenu);

	// It's possible that a play together event was sent by the system while the player was in-game or didn't
	// have the application launched. The game will automatically go directly to the main menu state in those cases
	// so this will handle Play Together if that is why we transitioned here.
	// if (PlayTogetherInfo.UserIndex != -1)
	// {
	// 	MainMenuUI->OnPlayTogetherEventReceived();
	// }


	RemoveNetworkFailureHandlers();
}

void UCaterGameInstance::EndMainMenuState()
{
	UIManager->SetUIState(ECaterUIState::NoMenu);
}

void UCaterGameInstance::BeginMessageMenuState()
{
	// if (PendingMessage.DisplayString.IsEmpty())
	// {
	// 	UE_LOG(LogOnlineGame, Warning, TEXT("UCaterGameInstance::BeginMessageMenuState: Display string is empty"));
	// 	GotoInitialState();
	// 	return;
	// }
	//
	// // Make sure we're not showing the loadscreen
	//
	//
	// check(!IsValid(MessageMenuUI));
	// MessageMenuUI = CreateWidget<UCaterMessageMenuWidget>(GetFirstGamePlayer());
	// MessageMenuUI->Construct(this, PendingMessage.PlayerOwner, PendingMessage.DisplayString, PendingMessage.OKButtonString, PendingMessage.CancelButtonString, PendingMessage.NextState);
	//
	// PendingMessage.DisplayString = FText::GetEmpty();
}

void UCaterGameInstance::EndMessageMenuState()
{

}

void UCaterGameInstance::BeginPlayingState()
{
	// Set presence for playing in a map
	SetPresenceForLocalPlayers(FString(TEXT("In Game")), FVariantData(FString(TEXT("InGame"))));

	//create in game menu
	ULocalPlayer* const Player = GetFirstGamePlayer();
	// UIManager->CreateInGameMenu(this, Player);

	// // Make sure viewport has focus
	// FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

void UCaterGameInstance::EndPlayingState()
{
	// Disallow splitscreen
	GetGameViewportClient()->SetForceDisableSplitscreen(true);

	// Clear the players' presence information
	SetPresenceForLocalPlayers(FString(TEXT("In Menu")), FVariantData(FString(TEXT("OnMenu"))));

	UWorld* const World = GetWorld();
	ACaterGameState* const GameState = World != NULL ? World->GetGameState<ACaterGameState>() : NULL;

	if (GameState)
	{
		// Send round end events for local players
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			ACaterPlayerController* CaterPC = Cast<ACaterPlayerController>(LocalPlayers[i]->PlayerController);
			if (CaterPC)
			{
				// Assuming you can't win if you quit early
				CaterPC->ClientSendRoundEndEvent(false, GameState->ElapsedTime);
			}
		}

		// Give the game state a chance to cleanup first
		GameState->RequestFinishAndExitToMainMenu();
	}
	else
	{
		// If there is no game state, make sure the session is in a good state
		// CleanupSessionOnReturnToMenu();
	}
	CleanupSessionOnReturnToMenu();
	
	// UIManager->DestructInGameMenu();
}

void UCaterGameInstance::InitMainMenu()
{
	GotoInitialState();
}

/// Online

ACaterGameSession* UCaterGameInstance::GetGameSession() const
{
	UWorld* const World = GetWorld();
	if (World)
	{
		AGameModeBase* const Game = World->GetAuthGameMode();
		if (Game)
		{
			return Cast<ACaterGameSession>(Game->GameSession);
		}
	}

	return nullptr;
}

void UCaterGameInstance::SetOnlineMode(EOnlineMode InOnlineMode)
{
	OnlineMode = InOnlineMode;
	UpdateUsingMultiplayerFeatures(InOnlineMode == EOnlineMode::Online);
}

void UCaterGameInstance::HandleNetworkConnectionStatusChanged(const FString& ServiceName,
                                                              EOnlineServerConnectionStatus::Type LastConnectionStatus,
                                                              EOnlineServerConnectionStatus::Type ConnectionStatus)
{
	UE_LOG(LogOnlineGame, Log, TEXT( "UCaterGameInstance::HandleNetworkConnectionStatusChanged: %s" ),
	       EOnlineServerConnectionStatus::ToString( ConnectionStatus ));
}

void UCaterGameInstance::HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType)
{
	UE_LOG(LogOnlineGame, Warning, TEXT( "UCaterGameInstance::HandleSessionFailure: %u" ), (uint32)FailureType);
}

void UCaterGameInstance::OnPreLoadMap(const FString& MapName)
{
	// if (bPendingEnableSplitscreen)
	// {
	// 	// Allow splitscreen
	// 	UGameViewportClient* GameViewportClient = GetGameViewportClient();
	// 	if (GameViewportClient != nullptr)
	// 	{
	// 		GameViewportClient->SetForceDisableSplitscreen(false);
	//
	// 		bPendingEnableSplitscreen = false;
	// 	}
	// }
}

void UCaterGameInstance::OnPostLoadMap(UWorld*)
{
	// // Make sure we hide the loading screen when the level is done loading
	// UCaterGameViewportClient * CaterViewport = Cast<UCaterGameViewportClient>(GetGameViewportClient());
	// if (CaterViewport != nullptr)
	// {
	// 	CaterViewport->HideLoadingScreen();
	// }
}

// void UCaterGameInstance::OnUserCanPlayInvite(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
// {
// 	CleanupOnlinePrivilegeTask();
// 	if (WelcomeMenuUI.IsValid())
// 	{
// 		WelcomeMenuUI->LockControls(false);
// 	}
//
// 	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)	
// 	{
// 		if (UserId == *PendingInvite.UserId)
// 		{
// 			PendingInvite.bPrivilegesCheckedAndAllowed = true;
// 		}		
// 	}
// 	else
// 	{
// 		DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
// 		GotoState(CaterGameInstanceState::WelcomeScreen);
// 	}
// }
//
// void UCaterGameInstance::OnUserCanPlayTogether(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
// {
// 	CleanupOnlinePrivilegeTask();
// 	if (WelcomeMenuUI.IsValid())
// 	{
// 		WelcomeMenuUI->LockControls(false);
// 	}
//
// 	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
// 	{
// 		if (WelcomeMenuUI.IsValid())
// 		{
// 			WelcomeMenuUI->SetControllerAndAdvanceToMainMenu(PlayTogetherInfo.UserIndex);
// 		}
// 	}
// 	else
// 	{
// 		DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
// 		GotoState(CaterGameInstanceState::WelcomeScreen);
// 	}
// }

void UCaterGameInstance::OnPostDemoPlay()
{
	GotoState(CaterGameInstanceState::Playing);
}


void UCaterGameInstance::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Log, TEXT("UCaterGameInstance::OnEndSessionComplete: Session=%s bWasSuccessful=%s"),
	       *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
			Sessions->ClearOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegateHandle);
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		}
	}

	// continue
	CleanupSessionOnReturnToMenu();
}

void UCaterGameInstance::CleanupSessionOnReturnToMenu()
{
	bool bPendingOnlineOp = false;

	// end online game and then destroy it
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Sessions = (OnlineSub != NULL) ? OnlineSub->GetSessionInterface() : NULL;

	if (Sessions.IsValid())
	{
		FName GameSession(NAME_GameSession);
		EOnlineSessionState::Type SessionState = Sessions->GetSessionState(NAME_GameSession);
		UE_LOG(LogOnline, Log, TEXT("Session %s is '%s'"), *GameSession.ToString(),
		       EOnlineSessionState::ToString(SessionState));

		if (EOnlineSessionState::InProgress == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Ending session %s on return to main menu"), *GameSession.ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(
				OnEndSessionCompleteDelegate);
			Sessions->EndSession(NAME_GameSession);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Ending == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Waiting for session %s to end on return to main menu"),
			       *GameSession.ToString());
			OnEndSessionCompleteDelegateHandle = Sessions->AddOnEndSessionCompleteDelegate_Handle(
				OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Ended == SessionState || EOnlineSessionState::Pending == SessionState)
		{
			UE_LOG(LogOnline, Log, TEXT("Destroying session %s on return to main menu"), *GameSession.ToString());
			OnDestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
				OnEndSessionCompleteDelegate);
			Sessions->DestroySession(NAME_GameSession);
			bPendingOnlineOp = true;
		}
		else if (EOnlineSessionState::Starting == SessionState || EOnlineSessionState::Creating == SessionState)
		{
			UE_LOG(LogOnline, Log,
			       TEXT("Waiting for session %s to start, and then we will end it to return to main menu"),
			       *GameSession.ToString());
			OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(
				OnEndSessionCompleteDelegate);
			bPendingOnlineOp = true;
		}
	}

	if (!bPendingOnlineOp)
	{
		//GEngine->HandleDisconnect( GetWorld(), GetWorld()->GetNetDriver() );
	}
}

void UCaterGameInstance::LabelPlayerAsQuitter(ULocalPlayer* LocalPlayer) const
{
	// ACaterPlayerState* const PlayerState = LocalPlayer && LocalPlayer->PlayerController ? Cast<ACaterPlayerState>(LocalPlayer->PlayerController->PlayerState) : nullptr;	
	// if(PlayerState)
	// {
	// 	PlayerState->SetQuitter(true);
	// }
}

void UCaterGameInstance::RemoveNetworkFailureHandlers()
{
	// Remove the local session/travel failure bindings if they exist
	if (GEngine->OnTravelFailure().IsBoundToObject(this) == true)
	{
		GEngine->OnTravelFailure().Remove(TravelLocalSessionFailureDelegateHandle);
	}
}

void UCaterGameInstance::AddNetworkFailureHandlers()
{
	// Add network/travel error handlers (if they are not already there)
	if (GEngine->OnTravelFailure().IsBoundToObject(this) == false)
	{
		TravelLocalSessionFailureDelegateHandle = GEngine->OnTravelFailure().AddUObject(
			this, &UCaterGameInstance::TravelLocalSessionFailure);
	}
}

// TSubclassOf<UOnlineSession> UCaterGameInstance::GetOnlineSessionClass()
// {
// 	return UCaterOnlineSessionClient::StaticClass();
// }

bool UCaterGameInstance::HostQuickSession(ULocalPlayer& LocalPlayer, const FOnlineSessionSettings& SessionSettings)
{
	// This function is different from BeginHostingQuickMatch in that it creates a session and then starts a quick match,
	// while BeginHostingQuickMatch assumes a session already exists

	if (ACaterGameSession* const GameSession = GetGameSession())
	{
		// Add callback delegate for completion
		OnCreatePresenceSessionCompleteDelegateHandle = GameSession->OnCreatePresenceSessionComplete().AddUObject(
			this, &UCaterGameInstance::OnCreatePresenceSessionComplete);

		TravelURL = GetQuickMatchUrl();

		FOnlineSessionSettings HostSettings = SessionSettings;

		const FString GameType = UGameplayStatics::ParseOption(TravelURL, TEXT("game"));

		// Determine the map name from the travelURL
		const FString MapNameSubStr = "/Game/Maps/";
		const FString ChoppedMapName = TravelURL.RightChop(MapNameSubStr.Len());
		const FString MapName = ChoppedMapName.LeftChop(ChoppedMapName.Len() - ChoppedMapName.Find("?game"));

		HostSettings.Set(SETTING_GAMEMODE, GameType, EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings.Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings.NumPublicConnections = 16;

		if (GameSession->HostSession(LocalPlayer.GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession,
		                             SessionSettings))
		{
			// If any error occurred in the above, pending state would be set
			if (PendingState == CurrentState || PendingState == CaterGameInstanceState::None)
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				GotoState(CaterGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}

bool UCaterGameInstance::HostGame(ULocalPlayer* LocalPlayer, const FString& GameType, const FString& InTravelURL)
{
	if (GetOnlineMode() == EOnlineMode::Offline)
	{
		//
		// Offline game, just go straight to map
		//

		GotoState(CaterGameInstanceState::Playing);

		// Travel to the specified match URL
		TravelURL = InTravelURL;
		GetWorld()->ServerTravel(TravelURL);
		return true;
	}

	//
	// Online game
	//

	ACaterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		// add callback delegate for completion
		OnCreatePresenceSessionCompleteDelegateHandle = GameSession->OnCreatePresenceSessionComplete().AddUObject(
			this, &UCaterGameInstance::OnCreatePresenceSessionComplete);

		TravelURL = InTravelURL;
		bool const bIsLanMatch = InTravelURL.Contains(TEXT("?bIsLanMatch"));

		//determine the map name from the travelURL
		const FString& MapNameSubStr = "/Game/Maps/";
		const FString& ChoppedMapName = TravelURL.RightChop(MapNameSubStr.Len());
		const FString& MapName = ChoppedMapName.LeftChop(ChoppedMapName.Len() - ChoppedMapName.Find("?game"));

		if (GameSession->HostSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession,
		                             GameType, MapName, bIsLanMatch, true, ACaterGameSession::DEFAULT_NUM_PLAYERS))
		{
			// If any error occurred in the above, pending state would be set
			if ((PendingState == CurrentState) || (PendingState == CaterGameInstanceState::None))
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				GotoState(CaterGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}

bool UCaterGameInstance::JoinSession(ULocalPlayer* LocalPlayer, int32 SessionIndexInSearchResults)
{
	// needs to tear anything down based on current state?

	ACaterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(
			this, &UCaterGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession,
		                             SessionIndexInSearchResults))
		{
			// If any error occured in the above, pending state would be set
			if ((PendingState == CurrentState) || (PendingState == CaterGameInstanceState::None))
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				GotoState(CaterGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}

bool UCaterGameInstance::JoinSession(ULocalPlayer* LocalPlayer, const FOnlineSessionSearchResult& SearchResult)
{
	// needs to tear anything down based on current state?
	ACaterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		AddNetworkFailureHandlers();

		OnJoinSessionCompleteDelegateHandle = GameSession->OnJoinSessionComplete().AddUObject(
			this, &UCaterGameInstance::OnJoinSessionComplete);
		if (GameSession->JoinSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession,
		                             SearchResult))
		{
			// If any error occured in the above, pending state would be set
			if ((PendingState == CurrentState) || (PendingState == CaterGameInstanceState::None))
			{
				// Go ahead and go into loading state now
				// If we fail, the delegate will handle showing the proper messaging and move to the correct state
				GotoState(CaterGameInstanceState::Playing);
				return true;
			}
		}
	}

	return false;
}


/** Callback which is intended to be called upon finding sessions */
void UCaterGameInstance::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	// unhook the delegate
	ACaterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnJoinSessionComplete().Remove(OnJoinSessionCompleteDelegateHandle);
	}

	// Add the splitscreen player if one exists
	if (Result == EOnJoinSessionCompleteResult::Success && LocalPlayers.Num() > 1)
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
		if (Sessions.IsValid() && LocalPlayers[1]->GetPreferredUniqueNetId().IsValid())
		{
			Sessions->RegisterLocalPlayer(*LocalPlayers[1]->GetPreferredUniqueNetId(), NAME_GameSession,
			                              FOnRegisterLocalPlayerCompleteDelegate::CreateUObject(
				                              this, &UCaterGameInstance::OnRegisterJoiningLocalPlayerComplete));
		}
	}
	else
	{
		// We either failed or there is only a single local user
		FinishJoinSession(Result);
	}
}

void UCaterGameInstance::FinishJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		FText ReturnReason;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Game is full.");
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Game no longer exists.");
			break;
		default:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Join failed.");
			break;
		}

		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		// ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	InternalTravelToSession(NAME_GameSession);
}

void UCaterGameInstance::OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId,
                                                              EOnJoinSessionCompleteResult::Type Result)
{
	FinishJoinSession(Result);
}

void UCaterGameInstance::InternalTravelToSession(const FName& SessionName)
{
	APlayerController* const PlayerController = GetFirstLocalPlayerController();

	if (PlayerController == nullptr)
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "InvalidPlayerController", "Invalid Player Controller");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		// ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	// travel to session
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());

	if (OnlineSub == nullptr)
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "OSSMissing", "OSS missing");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		RemoveNetworkFailureHandlers();
		// ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
		return;
	}

	FString URL;
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

	if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(SessionName, URL))
	{
		FText FailReason = NSLOCTEXT("NetworkErrors", "TravelSessionFailed", "Travel to Session failed.");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		// ShowMessageThenGoMain(FailReason, OKButton, FText::GetEmpty());
		UE_LOG(LogOnlineGame, Warning, TEXT("Failed to travel to session upon joining it"));
		return;
	}


	PlayerController->ClientTravel(URL, TRAVEL_Absolute);
}

/** Callback which is intended to be called upon session creation */
void UCaterGameInstance::OnCreatePresenceSessionComplete(FName SessionName, bool bWasSuccessful)
{
	ACaterGameSession* const GameSession = GetGameSession();
	if (GameSession)
	{
		GameSession->OnCreatePresenceSessionComplete().Remove(OnCreatePresenceSessionCompleteDelegateHandle);

		// Add the splitscreen player if one exists
		if (bWasSuccessful && LocalPlayers.Num() > 1)
		{
			IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
			if (Sessions.IsValid() && LocalPlayers[1]->GetPreferredUniqueNetId().IsValid())
			{
				Sessions->RegisterLocalPlayer(*LocalPlayers[1]->GetPreferredUniqueNetId(), NAME_GameSession,
				                              FOnRegisterLocalPlayerCompleteDelegate::CreateUObject(
					                              this, &UCaterGameInstance::OnRegisterLocalPlayerComplete));
			}
		}
		else
		{
			// We either failed or there is only a single local user
			FinishSessionCreation(bWasSuccessful
				                      ? EOnJoinSessionCompleteResult::Success
				                      : EOnJoinSessionCompleteResult::UnknownError);
		}
	}
}

/** Initiates the session searching */
bool UCaterGameInstance::FindSessions(ULocalPlayer* PlayerOwner, bool bIsDedicatedServer, bool bFindLAN)
{
	bool bResult = false;

	check(PlayerOwner != nullptr);
	if (PlayerOwner)
	{
		ACaterGameSession* const GameSession = GetGameSession();
		if (GameSession)
		{
			GameSession->OnFindSessionsComplete().RemoveAll(this);
			OnSearchSessionsCompleteDelegateHandle = GameSession->OnFindSessionsComplete().AddUObject(
				this, &UCaterGameInstance::OnSearchSessionsComplete);

			GameSession->FindSessions(PlayerOwner->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession,
			                          bFindLAN, !bIsDedicatedServer);

			bResult = true;
		}
	}

	return bResult;
}

/** Callback which is intended to be called upon finding sessions */
void UCaterGameInstance::OnSearchSessionsComplete(bool bWasSuccessful)
{
	ACaterGameSession* const Session = GetGameSession();
	if (Session)
	{
		Session->OnFindSessionsComplete().Remove(OnSearchSessionsCompleteDelegateHandle);
	}
}


bool UCaterGameInstance::HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	bool const bOpenSuccessful = Super::HandleOpenCommand(Cmd, Ar, InWorld);
	if (bOpenSuccessful)
	{
		GotoState(CaterGameInstanceState::Playing);
	}

	return bOpenSuccessful;
}

bool UCaterGameInstance::HandleDisconnectCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	bool const bDisconnectSuccessful = Super::HandleDisconnectCommand(Cmd, Ar, InWorld);
	if (bDisconnectSuccessful)
	{
		GotoState(CaterGameInstanceState::MainMenu);
	}

	return bDisconnectSuccessful;
}

bool UCaterGameInstance::HandleTravelCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	bool const bTravelSuccessful = Super::HandleTravelCommand(Cmd, Ar, InWorld);
	if (bTravelSuccessful)
	{
		GotoState(CaterGameInstanceState::Playing);
	}

	return bTravelSuccessful;
}


void UCaterGameInstance::HandleSignInChangeMessaging()
{
	// Master user signed out, go to initial state (if we aren't there already)
	if (CurrentState != GetInitialState())
	{
		GotoInitialState();
	}
}

void UCaterGameInstance::HandleUserLoginChanged(int32 GameUserIndex, ELoginStatus::Type PreviousLoginStatus,
                                                ELoginStatus::Type LoginStatus, const FUniqueNetId& UserId)
{
	// On Switch, accounts can play in LAN games whether they are signed in online or not. 
#if PLATFORM_SWITCH
	const bool bDowngraded = LoginStatus == ELoginStatus::NotLoggedIn || (GetOnlineMode() == EOnlineMode::Online && LoginStatus == ELoginStatus::UsingLocalProfile);
#else
	const bool bDowngraded = (LoginStatus == ELoginStatus::NotLoggedIn && GetOnlineMode() == EOnlineMode::Offline) || (
		LoginStatus != ELoginStatus::LoggedIn && GetOnlineMode() != EOnlineMode::Offline);
#endif

	UE_LOG(LogOnline, Log, TEXT( "HandleUserLoginChanged: bDownGraded: %i" ), (int)bDowngraded);

	// Find the local player associated with this unique net id
	ULocalPlayer* LocalPlayer = FindLocalPlayerFromUniqueNetId(UserId);

	LocalPlayerOnlineStatus[GameUserIndex] = LoginStatus;

	// If this user is signed out, but was previously signed in, punt to welcome (or remove splitscreen if that makes sense)
	if (LocalPlayer != NULL)
	{
		if (bDowngraded)
		{
			UE_LOG(LogOnline, Log, TEXT( "HandleUserLoginChanged: Player logged out: %s" ), *UserId.ToString());

			LabelPlayerAsQuitter(LocalPlayer);

			// Check to see if this was the master, or if this was a split-screen player on the client
			if (LocalPlayer == GetFirstGamePlayer() || GetOnlineMode() != EOnlineMode::Offline)
			{
				HandleSignInChangeMessaging();
			}
			else
			{
				// Remove local split-screen players from the list
				RemoveExistingLocalPlayer(LocalPlayer);
			}
		}
	}
}

void UCaterGameInstance::HandleAppWillDeactivate()
{
	if (CurrentState == CaterGameInstanceState::Playing)
	{
		// Just have the first player controller pause the game.
		UWorld* const GameWorld = GetWorld();
		if (GameWorld)
		{
			// protect against a second pause menu loading on top of an existing one if someone presses the Jewel / PS buttons.
			bool bNeedsPause = true;
			for (FConstControllerIterator It = GameWorld->GetControllerIterator(); It; ++It)
			{
				ACaterPlayerController* Controller = Cast<ACaterPlayerController>(*It);
				if (Controller && (Controller->IsPaused() || Controller->IsGameMenuVisible()))
				{
					bNeedsPause = false;
					break;
				}
			}

			if (bNeedsPause)
			{
				ACaterPlayerController* const Controller = Cast<ACaterPlayerController>(
					GameWorld->GetFirstPlayerController());
				if (Controller)
				{
					Controller->ShowInGameMenu();
				}
			}
		}
	}
}

void UCaterGameInstance::HandleAppSuspend()
{
	// Players will lose connection on resume. However it is possible the game will exit before we get a resume, so we must kick off round end events here.
	UE_LOG(LogOnline, Warning, TEXT( "UCaterGameInstance::HandleAppSuspend" ));
	UWorld* const World = GetWorld();
	ACaterGameState* const GameState = World != NULL ? World->GetGameState<ACaterGameState>() : NULL;

	if (CurrentState != CaterGameInstanceState::None && CurrentState != GetInitialState())
	{
		UE_LOG(LogOnline, Warning, TEXT( "UCaterGameInstance::HandleAppSuspend: Sending round end event for players" ));

		// Send round end events for local players
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			ACaterPlayerController* CaterPC = Cast<ACaterPlayerController>(LocalPlayers[i]->PlayerController);
			if (CaterPC && GameState)
			{
				// Assuming you can't win if you quit early
				CaterPC->ClientSendRoundEndEvent(false, GameState->ElapsedTime);
			}
		}
	}
}

void UCaterGameInstance::HandleAppResume()
{
	UE_LOG(LogOnline, Log, TEXT( "UCaterGameInstance::HandleAppResume" ));

	if (CurrentState != CaterGameInstanceState::None && CurrentState != GetInitialState())
	{
		UE_LOG(LogOnline, Warning, TEXT( "UCaterGameInstance::HandleAppResume: Attempting to sign out players" ));

		for (int32 i = 0; i < LocalPlayers.Num(); ++i)
		{
			if (LocalPlayers[i]->GetCachedUniqueNetId().IsValid() && LocalPlayerOnlineStatus[i] ==
				ELoginStatus::LoggedIn && !IsLocalPlayerOnline(LocalPlayers[i]))
			{
				UE_LOG(LogOnline, Log, TEXT( "UCaterGameInstance::HandleAppResume: Signed out during resume." ));
				HandleSignInChangeMessaging();
				break;
			}
		}
	}
}


void UCaterGameInstance::HandleSafeFrameChanged()
{
	UCanvas::UpdateAllCanvasSafeZoneData();
}

void UCaterGameInstance::RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer)
{
	check(ExistingPlayer);
	if (ExistingPlayer->PlayerController != NULL)
	{
		// Kill the player
		ACaterCharacter* MyPawn = Cast<ACaterCharacter>(ExistingPlayer->PlayerController->GetPawn());
		if (MyPawn)
		{
			MyPawn->KilledBy(NULL);
		}
	}

	// Remove local split-screen players from the list
	RemoveLocalPlayer(ExistingPlayer);
}

FReply UCaterGameInstance::OnPairingUsePreviousProfile()
{
	// Do nothing (except hide the message) if they want to continue using previous profile
	// UCaterGameViewportClient * CaterViewport = Cast<UCaterGameViewportClient>(GetGameViewportClient());
	//
	// if ( CaterViewport != nullptr )
	// {
	// 	CaterViewport->HideDialog();
	// }

	return FReply::Handled();
}

FReply UCaterGameInstance::OnPairingUseNewProfile()
{
	HandleSignInChangeMessaging();
	return FReply::Handled();
}

void UCaterGameInstance::HandleControllerPairingChanged(int GameUserIndex,
                                                        FControllerPairingChangedUserInfo PreviousUserInfo,
                                                        FControllerPairingChangedUserInfo NewUserInfo)
{
	UE_LOG(LogOnlineGame, Log,
	       TEXT("UCaterGameInstance::HandleControllerPairingChanged GameUserIndex %d PreviousUser '%s' NewUser '%s'"),
	       GameUserIndex, *PreviousUserInfo.User.ToString(), *PreviousUserInfo.User.ToString());
}

void UCaterGameInstance::HandleControllerConnectionChange(bool bIsConnection, int32 Unused, int32 GameUserIndex)
{
	UE_LOG(LogOnlineGame, Log,
	       TEXT("UCaterGameInstance::HandleControllerConnectionChange bIsConnection %d GameUserIndex %d"),
	       bIsConnection, GameUserIndex);

	if (!bIsConnection)
	{
		// Controller was disconnected

		// Find the local player associated with this user index
		ULocalPlayer* LocalPlayer = FindLocalPlayerFromControllerId(GameUserIndex);

		if (LocalPlayer == NULL)
		{
			return; // We don't care about players we aren't tracking
		}

		// Invalidate this local player's controller id.
		LocalPlayer->SetControllerId(-1);
	}
}

FReply UCaterGameInstance::OnControllerReconnectConfirm()
{
	return FReply::Handled();
}

TSharedPtr<const FUniqueNetId> UCaterGameInstance::GetUniqueNetIdFromControllerId(const int ControllerId)
{
	IOnlineIdentityPtr OnlineIdentityInt = Online::GetIdentityInterface(GetWorld());

	if (OnlineIdentityInt.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UniqueId = OnlineIdentityInt->GetUniquePlayerId(ControllerId);

		if (UniqueId.IsValid())
		{
			return UniqueId;
		}
	}

	return nullptr;
}


void UCaterGameInstance::UpdateUsingMultiplayerFeatures(bool bIsUsingMultiplayerFeatures)
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());

	if (OnlineSub)
	{
		for (int32 i = 0; i < LocalPlayers.Num(); ++i)
		{
			ULocalPlayer* LocalPlayer = LocalPlayers[i];

			FUniqueNetIdRepl PlayerId = LocalPlayer->GetPreferredUniqueNetId();
			if (PlayerId.IsValid())
			{
				OnlineSub->SetUsingMultiplayerFeatures(*PlayerId, bIsUsingMultiplayerFeatures);
			}
		}
	}
}

void UCaterGameInstance::TravelToSession(const FName& SessionName)
{
	// Added to handle failures when joining using quickmatch (handles issue of joining a game that just ended, i.e. during game ending timer)
	AddNetworkFailureHandlers();
	GotoState(CaterGameInstanceState::Playing);
	InternalTravelToSession(SessionName);
}

void UCaterGameInstance::SetIgnorePairingChangeForControllerId(const int32 ControllerId)
{
	IgnorePairingChangeForControllerId = ControllerId;
}

bool UCaterGameInstance::IsLocalPlayerOnline(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == NULL)
	{
		return false;
	}
	const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
		if (IdentityInterface.IsValid())
		{
			FUniqueNetIdRepl UniqueId = LocalPlayer->GetCachedUniqueNetId();
			if (UniqueId.IsValid())
			{
				const ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(*UniqueId);
				if (LoginStatus == ELoginStatus::LoggedIn)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool UCaterGameInstance::IsLocalPlayerSignedIn(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == NULL)
	{
		return false;
	}

	const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
		if (IdentityInterface.IsValid())
		{
			FUniqueNetIdRepl UniqueId = LocalPlayer->GetCachedUniqueNetId();
			if (UniqueId.IsValid())
			{
				return true;
			}
		}
	}

	return false;
}

bool UCaterGameInstance::ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer)
{
	if (!IsLocalPlayerOnline(LocalPlayer))
	{
		UE_LOG(LogOnline, Warning, TEXT("UCaterGameInstance::ValidatePlayerForOnlinePlay returned false"));
		return false;
	}

	return true;
}

bool UCaterGameInstance::ValidatePlayerIsSignedIn(ULocalPlayer* LocalPlayer)
{
	if (!IsLocalPlayerSignedIn(LocalPlayer))
	{
		UE_LOG(LogOnline, Warning, TEXT("UCaterGameInstance::ValidatePlayerIsSignedIn returned false"));
		return false;
	}

	return true;
}


FReply UCaterGameInstance::OnConfirmGeneric()
{
	// UCaterGameViewportClient * CaterViewport = Cast<UCaterGameViewportClient>(GetGameViewportClient());
	// if(CaterViewport)
	// {
	// 	CaterViewport->HideDialog();
	// }

	return FReply::Handled();
}

// void UCaterGameInstance::StartOnlinePrivilegeTask(const IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate& Delegate, EUserPrivileges::Type Privilege, TSharedPtr< const FUniqueNetId > UserId)
// {
// 	WaitMessageWidget = SNew(SCaterWaitDialog)
// 		.MessageText(NSLOCTEXT("NetworkStatus", "CheckingPrivilegesWithServer", "Checking privileges with server.  Please wait..."));
//
// 	if (GEngine && GEngine->GameViewport)
// 	{
// 		UGameViewportClient* const GVC = GEngine->GameViewport;
// 		GVC->AddViewportWidgetContent(WaitMessageWidget.ToSharedRef());
// 	}
//
// 	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetWorld());
// 	if (Identity.IsValid() && UserId.IsValid())
// 	{		
// 		Identity->GetUserPrivilege(*UserId, Privilege, Delegate);
// 	}
// 	else
// 	{
// 		// Can only get away with faking the UniqueNetId here because the delegates don't use it
// 		Delegate.ExecuteIfBound(FUniqueNetIdString(), Privilege, (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures);
// 	}
// }
//
// void UCaterGameInstance::CleanupOnlinePrivilegeTask()
// {
// 	if (GEngine && GEngine->GameViewport && WaitMessageWidget.IsValid())
// 	{
// 		UGameViewportClient* const GVC = GEngine->GameViewport;
// 		GVC->RemoveViewportWidgetContent(WaitMessageWidget.ToSharedRef());
// 	}
// }

// void UCaterGameInstance::DisplayOnlinePrivilegeFailureDialogs(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
// {	
// 	// Show warning that the user cannot play due to age restrictions
// 	UCaterGameViewportClient * CaterViewport = Cast<UCaterGameViewportClient>(GetGameViewportClient());
// 	TWeakObjectPtr<ULocalPlayer> OwningPlayer;
// 	if (GEngine)
// 	{
// 		for (auto It = GEngine->GetLocalPlayerIterator(GetWorld()); It; ++It)
// 		{
// 			FUniqueNetIdRepl OtherId = (*It)->GetPreferredUniqueNetId();
// 			if (OtherId.IsValid())
// 			{
// 				if (UserId == (*OtherId))
// 				{
// 					OwningPlayer = *It;
// 				}
// 			}
// 		}
// 	}
// 	
// 	if (CaterViewport != NULL && OwningPlayer.IsValid())
// 	{
// 		if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::AccountTypeFailure) != 0)
// 		{
// 			IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface(GetWorld());
// 			if (ExternalUI.IsValid())
// 			{
// 				ExternalUI->ShowAccountUpgradeUI(UserId);
// 			}
// 		}
// 		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate) != 0)
// 		{
// 			CaterViewport->ShowDialog(
// 				OwningPlayer.Get(),
// 				ECaterDialogType::Generic,
// 				NSLOCTEXT("OnlinePrivilegeResult", "RequiredSystemUpdate", "A required system update is available.  Please upgrade to access online features."),
// 				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
// 				FText::GetEmpty(),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric)
// 				);
// 		}
// 		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable) != 0)
// 		{
// 			CaterViewport->ShowDialog(
// 				OwningPlayer.Get(),
// 				ECaterDialogType::Generic,
// 				NSLOCTEXT("OnlinePrivilegeResult", "RequiredPatchAvailable", "A required game patch is available.  Please upgrade to access online features."),
// 				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
// 				FText::GetEmpty(),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric)
// 				);
// 		}
// 		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::AgeRestrictionFailure) != 0)
// 		{
// 			CaterViewport->ShowDialog(
// 				OwningPlayer.Get(),
// 				ECaterDialogType::Generic,
// 				NSLOCTEXT("OnlinePrivilegeResult", "AgeRestrictionFailure", "Cannot play due to age restrictions!"),
// 				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
// 				FText::GetEmpty(),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric)
// 				);
// 		}
// 		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::UserNotFound) != 0)
// 		{
// 			CaterViewport->ShowDialog(
// 				OwningPlayer.Get(),
// 				ECaterDialogType::Generic,
// 				NSLOCTEXT("OnlinePrivilegeResult", "UserNotFound", "Cannot play due invalid user!"),
// 				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
// 				FText::GetEmpty(),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric)
// 				);
// 		}
// 		else if ((PrivilegeResults & (uint32)IOnlineIdentity::EPrivilegeResults::GenericFailure) != 0)
// 		{
// 			CaterViewport->ShowDialog(
// 				OwningPlayer.Get(),
// 				ECaterDialogType::Generic,
// 				NSLOCTEXT("OnlinePrivilegeResult", "GenericFailure", "Cannot play online.  Check your network connection."),
// 				NSLOCTEXT("DialogButtons", "OKAY", "OK"),
// 				FText::GetEmpty(),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric),
// 				FOnClicked::CreateUObject(this, &UCaterGameInstance::OnConfirmGeneric)
// 				);
// 		}
// 	}
// }

void UCaterGameInstance::OnRegisterLocalPlayerComplete(const FUniqueNetId& PlayerId,
                                                       EOnJoinSessionCompleteResult::Type Result)
{
	FinishSessionCreation(Result);
}

void UCaterGameInstance::FinishSessionCreation(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		// This will send any Play Together invites if necessary, or do nothing.
		// SendPlayTogetherInvites();

		// Travel to the specified match URL
		GetWorld()->ServerTravel(TravelURL);
	}
	else
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "CreateSessionFailed", "Failed to create session.");
		FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		// ShowMessageThenGoMain(ReturnReason, OKButton, FText::GetEmpty());
	}
}

FString UCaterGameInstance::GetQuickMatchUrl()
{
	static const FString QuickMatchUrl(TEXT("/Game/Maps/Highrise?game=TDM?listen"));
	return QuickMatchUrl;
}

void UCaterGameInstance::BeginHostingQuickMatch()
{
	GotoState(CaterGameInstanceState::Playing);

	// Travel to the specified match URL
	GetWorld()->ServerTravel(GetQuickMatchUrl());
}

// void UCaterGameInstance::OnPlayTogetherEventReceived(const int32 UserIndex, const TArray<TSharedPtr<const FUniqueNetId>>& UserIdList)
// {
// 	PlayTogetherInfo = FCaterPlayTogetherInfo(UserIndex, UserIdList);
//
// 	const IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
// 	check(OnlineSub);
//
// 	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
// 	check(SessionInterface.IsValid());
//
// 	// If we have available slots to accomedate the whole party in our current sessions, we should send invites to the existing one
// 	// instead of a new one according to Sony's best practices.
// 	const FNamedOnlineSession* const Session = SessionInterface->GetNamedSession(NAME_GameSession);
// 	if (Session != nullptr && Session->NumOpenPrivateConnections + Session->NumOpenPublicConnections >= UserIdList.Num())
// 	{
// 		SendPlayTogetherInvites();
// 	}
// 	// Always handle Play Together in the main menu since the player has session customization options.
// 	else if (CurrentState == CaterGameInstanceState::MainMenu)
// 	{
// 		MainMenuUI->OnPlayTogetherEventReceived();
// 	}
// 	else if (CurrentState == CaterGameInstanceState::WelcomeScreen)
// 	{
// 		StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &UCaterGameInstance::OnUserCanPlayTogether), EUserPrivileges::CanPlayOnline, PendingInvite.UserId);
// 	}
// 	else
// 	{
// 		GotoState(CaterGameInstanceState::MainMenu);
// 	}
// }

// void UCaterGameInstance::SendPlayTogetherInvites()
// {
// 	const IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
// 	check(OnlineSub);
//
// 	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
// 	check(SessionInterface.IsValid());
//
// 	if (PlayTogetherInfo.UserIndex != -1)
// 	{
// 		for (const ULocalPlayer* LocalPlayer : LocalPlayers)
// 		{
// 			if (LocalPlayer->GetControllerId() == PlayTogetherInfo.UserIndex)
// 			{
// 				FUniqueNetIdRepl PlayerId = LocalPlayer->GetPreferredUniqueNetId();
// 				if (PlayerId.IsValid())
// 				{
// 					// Automatically send invites to friends in the player's PS4 party to conform with Play Together requirements
// 					for (const TSharedPtr<const FUniqueNetId>& FriendId : PlayTogetherInfo.UserIdList)
// 					{
// 						SessionInterface->SendSessionInviteToFriend(*PlayerId, NAME_GameSession, *FriendId.ToSharedRef());
// 					}
// 				}
//
// 			}
// 		}
//
// 		PlayTogetherInfo = FCaterPlayTogetherInfo();
// 	}
// }
