// Fill out your copyright notice in the Description page of Project Settings.


#include "EndMatchWidget.h"

#include "CaterGameInstance.h"
#include "CaterGameState.h"
#include "CaterUIManager.h"
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
	//if is play in editor
	if(GetWorld()->IsPlayInEditor())
	{
		// GI->GotoState(CaterGameInstanceState::MainMenu);
		GI->EndPlayingState();
		FString const StartURL = FString::Printf(TEXT("/Game/Maps/%s?game=%s%s"),
										 L"TopDownExampleMap",
										 GI->GetOnlineMode() != EOnlineMode::Offline
											 ? TEXT("?listen")
											 : TEXT(""),
										 GI->GetOnlineMode() == EOnlineMode::LAN
											 ? TEXT("?bIsLanMatch")
											 : TEXT(""));
		FString const GameType = TEXT("");
		GI->HostGame(GI->GetFirstGamePlayer(), GameType, StartURL);
		GI->BeginPlayingState();
		

	}
	else
	{
		//restart level
		UGameplayStatics::OpenLevel(GI->GetWorld(), FName(*GI->GetWorld()->GetName()), false);
	}
}

void UEndMatchWidget::OnClickContinueButton()
{
	GI->GetUIManager()->SetUIState(ECaterUIState::NoMenu);
}
