// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "CaterGameState.generated.h"

UENUM()
enum class ECaterGameEnding
{
	PlayerFallen,
	PlayerFinished
};

UCLASS()
class ACaterGameState : public AGameState
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CaterGameState")
	TArray<class ACaterCharacter*> FinishedPlayers;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CaterGameState")
	uint8 bIsGameFinished : 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "CaterGameState")
	ECaterGameEnding GameEnding;

	void RequestFinishAndExitToMainMenu();

	UFUNCTION(Reliable, Client, Category = "CaterGameState")
	void ClientOnGameFinished();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const override;
};
