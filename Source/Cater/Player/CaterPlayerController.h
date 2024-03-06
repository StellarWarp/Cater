// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CaterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CaterPlayerController.generated.h"

class UCaterUIManager;
class UCaterInGameMenuWidget;

UCLASS()
class ACaterPlayerController : public APlayerController
{
	GENERATED_BODY()
protected:

	bool bAllowGameActions;

	UPROPERTY()
	ACaterHUD* CaterHUD;

	FTimerHandle TimerHandle_ClientStartOnlineGame;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	uint32 bMoving : 1;
	FVector Destination;


	// Begin PlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

public:
	ACaterPlayerController();
	/** sets spectator location and rotation */
	// UFUNCTION(reliable, client)
	// void ClientSetSpectatorCamera(FVector CameraLocation, FRotator CameraRotation);

	

	/** notify player about started match */
	UFUNCTION(reliable, client)
	void ClientGameStarted();

	/** Starts the online game using the session name in the PlayerState */
	UFUNCTION(reliable, client)
	void ClientStartOnlineGame();

	/** Ends the online game using the session name in the PlayerState */
	UFUNCTION(reliable, client)
	void ClientEndOnlineGame();
	

	void ClientSendRoundEndEvent(bool bCond, int32 ElapsedTime);

	UFUNCTION(reliable, client)
	void ClientOnGameFinished();

	/// UI
protected:
	UPROPERTY()
	UCaterUIManager* UIManager;

public:
	UCaterUIManager* GetUIManager();
	ACaterHUD* GetCaterHUD();
	bool IsGameMenuVisible();
	void ShowInGameMenu();
	void OnToggleInGameMenu();
	void ForceExitGame();

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);
	
	void MoveToDestination();

	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Common request to nagivate player to the given world location. */
	void RequestSetNewMoveDestination(const FVector DestLocation);

	/** Call navigate player to the given world location (Client Version). */
	UFUNCTION(Reliable, Client)
	void ClientSetNewMoveDestination(const FVector DestLocation);

	/** Call navigate player to the given world location (Server Version). */
	UFUNCTION(Reliable, Server)
	void ServerSetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();
};
