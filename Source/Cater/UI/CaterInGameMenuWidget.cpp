#include "CaterInGameMenuWidget.h"

#include "CaterGameInstance.h"
#include "CaterUIManager.h"

void UCaterInGameMenuWidget::Construct(UCaterGameInstance* CaterGameInstance, ULocalPlayer* LocalPlayer)
{
	check(CaterGameInstance);
	GameInstance = CaterGameInstance;
	PlayerOwner = LocalPlayer;
}

void UCaterInGameMenuWidget::OnClickServePauseGame()
{
}



void UCaterInGameMenuWidget::OnClickServerResumeGame()
{
}

void UCaterInGameMenuWidget::OnClickCloseMenu()
{
    GameInstance->GetUIManager()->SetUIState(ECaterUIState::NoMenu);
}

void UCaterInGameMenuWidget::OnClickMainMenu()
{
    GameInstance->GotoState(CaterGameInstanceState::MainMenu);
}
