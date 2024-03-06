// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CaterCharacter.h"
#include "GameFramework/GameMode.h"
#include "CaterGameMode.generated.h"

class ACaterPlayerState;
/**
 * 
 */
UCLASS()
class ACaterGameMode : public AGameMode
{
	GENERATED_BODY()

	ACaterGameMode();

	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

public:
	void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType);
	FName GetMatchState();
	
	void EndMatch() override;

	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void OnPlayerFallen(ACaterCharacter* FallenPlayer);

	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void OnPlayerRespawn(ACaterCharacter* RespawnedPlayer);

	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void OnPlayerEnterFinishZone(ACaterCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void OnPlayerExitFinishZone(ACaterCharacter* Player);
};
