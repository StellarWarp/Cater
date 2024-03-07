// Fill out your copyright notice in the Description page of Project Settings.


#include "CaterPlayerController.h"

#include "CaterCharacter.h"
#include "CaterGameInstance.h"
#include "CaterGameState.h"
#include "CaterHUD.h"
#include "CaterInGameMenuWidget.h"
#include "CaterPlayerState.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "CaterUIManager.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/DecalComponent.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"


UCaterUIManager* ACaterPlayerController::GetUIManager()
{
	if (!UIManager)
	{
		UCaterGameInstance* SGI = Cast<UCaterGameInstance>(GetWorld()->GetGameInstance());
		UIManager = SGI->GetUIManager();
	}
	return UIManager;
}

ACaterHUD* ACaterPlayerController::GetCaterHUD()
{
	return Cast<ACaterHUD>(GetHUD());
}

bool ACaterPlayerController::IsGameMenuVisible()
{
	return GetUIManager()->GetInGameMenuUI()->IsInViewport();
}

void ACaterPlayerController::ShowInGameMenu()
{
	GetUIManager()->GetInGameMenuUI()->AddToViewport();
}

void ACaterPlayerController::ClientSendRoundEndEvent(bool bCond, int32 ElapsedTime)
{
}

void ACaterPlayerController::ClientGameStarted_Implementation()
{
	bAllowGameActions = true;

	// Enable controls mode now the game has started
	SetIgnoreMoveInput(false);

	// ACaterHUD* CaterHUD = GetCaterHUD();
	// if (CaterHUD)
	// {
	// 	CaterHUD->SetMatchState(ECaterMatchState::Playing);
	// 	CaterHUD->ShowScoreboard(false);
	// }
	// bGameEndedFrame = false;
	//
	// QueryAchievements();
	//
	// QueryStats();

	const UWorld* World = GetWorld();

	// Send round start event
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if (LocalPlayer != nullptr && World != nullptr && Events.IsValid())
	{
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			// Generate a new session id
			Events->SetPlayerSessionId(*UniqueId, FGuid::NewGuid());

			FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());

			// Fire session start event for all cases
			FOnlineEventParms Params;
			Params.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // @todo determine game mode (ffa v tdm)
			Params.Add(TEXT("DifficultyLevelId"), FVariantData((int32)0)); // unused
			Params.Add(TEXT("MapName"), FVariantData(MapName));

			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionStart"), Params);

			// Online matches require the MultiplayerRoundStart event as well
			UCaterGameInstance* SGI = Cast<UCaterGameInstance>(World->GetGameInstance());

			if (SGI && (SGI->GetOnlineMode() == EOnlineMode::Online))
			{
				FOnlineEventParms MultiplayerParams;

				// @todo: fill in with real values
				MultiplayerParams.Add(TEXT("SectionId"), FVariantData((int32)0)); // unused
				MultiplayerParams.Add(TEXT("GameplayModeId"), FVariantData((int32)1));
				// @todo determine game mode (ffa v tdm)
				MultiplayerParams.Add(TEXT("MatchTypeId"), FVariantData((int32)1));
				// @todo abstract the specific meaning of this value across platforms
				MultiplayerParams.Add(TEXT("DifficultyLevelId"), FVariantData((int32)0)); // unused

				Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundStart"), MultiplayerParams);
			}


			// bHasSentStartEvents = true;
		}
	}
}

/** Starts the online game using the session name in the PlayerState */
void ACaterPlayerController::ClientStartOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	ACaterPlayerState* CaterPlayerState = Cast<ACaterPlayerState>(PlayerState);
	if (CaterPlayerState)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && (Sessions->GetNamedSession(CaterPlayerState->SessionName) != nullptr))
			{
				UE_LOG(LogOnline, Log, TEXT("Starting session %s on client"),
				       *CaterPlayerState->SessionName.ToString());
				Sessions->StartSession(CaterPlayerState->SessionName);
			}
		}
	}
	else
	{
		// Keep retrying until player state is replicated
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ClientStartOnlineGame, this,
		                                       &ACaterPlayerController::ClientStartOnlineGame_Implementation, 0.2f,
		                                       false);
	}
}

/** Ends the online game using the session name in the PlayerState */
void ACaterPlayerController::ClientEndOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	ACaterPlayerState* CaterPlayerState = Cast<ACaterPlayerState>(PlayerState);
	if (CaterPlayerState)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && (Sessions->GetNamedSession(CaterPlayerState->SessionName) != nullptr))
			{
				UE_LOG(LogOnline, Log, TEXT("Ending session %s on client"), *CaterPlayerState->SessionName.ToString());
				Sessions->EndSession(CaterPlayerState->SessionName);
			}
		}
	}
}


ACaterPlayerController::ACaterPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ACaterPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}

	if (bMoving)
	{
		MoveToDestination();
	}
}

void ACaterPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &ACaterPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &ACaterPlayerController::OnSetDestinationReleased);

	// support touch devices 
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &ACaterPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &ACaterPlayerController::MoveToTouchLocation);
	
	InputComponent->BindAction("InGameMenu", IE_Pressed, this, &ACaterPlayerController::OnToggleInGameMenu);
	InputComponent->BindAction("ForceExitGame", IE_Pressed, this, &ACaterPlayerController::ForceExitGame);
	// InputComponent->BindAction("Scoreboard", IE_Pressed, this, &AShooterPlayerController::OnShowScoreboard);
}

void ACaterPlayerController::OnToggleInGameMenu()
{
	GetUIManager()->ToggleInGameMenu();
}

void ACaterPlayerController::ForceExitGame()
{
	Cast<UCaterGameInstance>(GetGameInstance())->GotoState(CaterGameInstanceState::MainMenu);
}

void ACaterPlayerController::MoveToMouseCursor()
{
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		if (ACaterCharacter* MyPawn = Cast<ACaterCharacter>(GetPawn()))
		{
			if (MyPawn->GetCursorToWorld())
			{
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(
					this, MyPawn->GetCursorToWorld()->GetComponentLocation());
			}
		}
	}
	else
	{
		// Trace to see what is under the mouse cursor
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, false, Hit);

		if (Hit.bBlockingHit)
		{
			// RequestSetNewMoveDestination(Hit.ImpactPoint);
			// We hit something, move there
			// SetNewMoveDestination(Hit.ImpactPoint);
			bMoving = true;
			Destination = Hit.ImpactPoint;
		}
	}
}

void ACaterPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

// Common destination setting and movement implementation.
void ACaterPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	if (APawn* const MyPawn = GetPawn())
	{
		float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly.
		if (Distance > 120.0f)
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
	}
}

void ACaterPlayerController::MoveToDestination()
{
	check(bMoving);
	const auto PawnPtr = GetPawn();
	if(!IsValid(PawnPtr))
	{
		UE_LOG(LogTemp, Error, TEXT("Pawn is not valid"));
		return;
	}
	FVector Direction = (Destination - PawnPtr->GetActorLocation()).GetUnsafeNormal();
	PawnPtr->AddMovementInput(Direction, 1.0f);
	if (FVector::Dist(Destination, GetPawn()->GetActorLocation()) < 120.0f)
	{
		bMoving = false;
	}
}

// Requests a destination set for the client and server.
void ACaterPlayerController::RequestSetNewMoveDestination(const FVector DestLocation)
{
	ClientSetNewMoveDestination(DestLocation);
	ServerSetNewMoveDestination(DestLocation);
}

// Requests a destination set for the client (Comes first, since client calls it by clicking).
void ACaterPlayerController::ClientSetNewMoveDestination_Implementation(const FVector DestLocation)
{
	SetNewMoveDestination(DestLocation);
	//log screen location
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Client Screen Location: %s"), *DestLocation.ToString()));
}

// Requests a destination set for the server (Comes after, to replicate client movement server-side).
void ACaterPlayerController::ServerSetNewMoveDestination_Implementation(const FVector DestLocation)
{
	SetNewMoveDestination(DestLocation);
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Server Screen Location: %s"), *DestLocation.ToString()));
}


void ACaterPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

void ACaterPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}
