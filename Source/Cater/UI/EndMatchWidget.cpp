// Fill out your copyright notice in the Description page of Project Settings.


#include "EndMatchWidget.h"

#include "CaterGameInstance.h"
#include "CaterGameState.h"
#include "Kismet/GameplayStatics.h"

void UEndMatchWidget::Construct(UCaterGameInstance* GameInstance)
{
	this->GI = GameInstance;
	auto GS = Cast<ACaterGameState>(UGameplayStatics::GetGameState(GetWorld()));
	if(!GS)
	{
		//log error and ret
		UE_LOG(LogOnline, Error, TEXT("UEndMatchWidget::Construct: GameState is null"));
		return;
	}
	switch (GS->GameEnding) {
	case ECaterGameEnding::PlayerFallen:
		EndGameMessage = L"Player has fallen!";
		break;
	case ECaterGameEnding::PlayerFinished:
		EndGameMessage = L"Player has finished!";
		break;
	}
}

void UEndMatchWidget::OnExitToMainMenuButtonClicked()
{
	GI->GotoState(CaterGameInstanceState::MainMenu);
}

void UEndMatchWidget::OnRematchButtonClicked()
{
	
}

void UEndMatchWidget::OnClickContinueButton()
{
	
}
