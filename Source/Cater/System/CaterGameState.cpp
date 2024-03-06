// Fill out your copyright notice in the Description page of Project Settings.


#include "CaterGameState.h"

#include "CaterGameInstance.h"
#include "CaterUIManager.h"
#include "Net/UnrealNetwork.h"

void ACaterGameState::RequestFinishAndExitToMainMenu()
{
	bReplicates = true;
}

void ACaterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACaterGameState, FinishedPlayers);
	DOREPLIFETIME(ACaterGameState, bIsGameFinished);
	DOREPLIFETIME(ACaterGameState, GameEnding);
}

void ACaterGameState::ClientOnGameFinished_Implementation()
{
	const auto UI = Cast<UCaterGameInstance>(GetGameInstance())->GetUIManager();
	if (!UI)
	{
		//log error and ret
		UE_LOG(LogOnline, Error, TEXT("ACaterGameState::ClientOnGameFinished: UI is null"));
		return;
	}
	UI->SetUIState(ECaterUIState::EndMatchMenu);
	

}
