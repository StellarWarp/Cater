// Fill out your copyright notice in the Description page of Project Settings.


#include "CaterUIManager.h"

#include "CaterMainMenuWidget.h"
#include "OnlineSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "CaterGameInstance.h"
#include "CaterInGameMenuWidget.h"
#include "EndMatchWidget.h"

void UCaterUIManager::CreateMainMenu(ULocalPlayer* Player)
{
	MainMenuUI = CreateWidget<UCaterMainMenuWidget>(Player->GetPlayerController(GetWorld()), MainMenuUIClass);
	if(MainMenuUI == nullptr)
	{
		UE_LOG(LogOnline, Error, TEXT("UCaterGameInstance::BeginMainMenuState: MainMenuUI is null") );
		return;
	}
	MainMenuUI->Construct(GI, Player);
	MainMenuUI->AddToViewport();
}

void UCaterUIManager::DestructMainMenu()
{
	MainMenuUI->RemoveFromViewport();
	MainMenuUI->Destruct();
	MainMenuUI = nullptr;
}

void UCaterUIManager::CreateInGameMenu(ULocalPlayer* Player)
{
	InGameMenuUI = CreateWidget<UCaterInGameMenuWidget>(Player->GetPlayerController(GetWorld()), InGameMenuUIClass);
	if(InGameMenuUI == nullptr)
	{
		UE_LOG(LogOnline, Error, TEXT("UCaterGameInstance::BeginMainMenuState: InGameMenuUI is null") );
		return;
	}
	InGameMenuUI->Construct(GI, Player);
	InGameMenuUI->AddToViewport();
}

void UCaterUIManager::DestructInGameMenu()
{
	InGameMenuUI->RemoveFromViewport();
	InGameMenuUI->Destruct();
	InGameMenuUI = nullptr;
}

void UCaterUIManager::ToggleInGameMenu()
{
	if (InGameMenuUI == nullptr)
	{
		SetUIState(ECaterUIState::InGameMenu);
	}
	else
	{
		SetUIState(ECaterUIState::NoMenu);
	}
}

void UCaterUIManager::CreateMessageMenu(ULocalPlayer* Player)
{
}

void UCaterUIManager::DestructMessageMenu()
{
}

void UCaterUIManager::CreateEndMatchMenu()
{
	EndMatchUI = CreateWidget<UEndMatchWidget>(GI->GetFirstGamePlayer()->GetPlayerController(GetWorld()), EndMatchUIClass);
	if(EndMatchUI == nullptr)
	{
		UE_LOG(LogOnline, Error, TEXT("UCaterGameInstance::BeginMainMenuState: EndMatchUI is null") );
		return;
	}
	EndMatchUI->Construct(GI);
	EndMatchUI->AddToViewport();
}

void UCaterUIManager::DestructEndMatchMenu()
{
	EndMatchUI->RemoveFromViewport();
	EndMatchUI->Destruct();
	EndMatchUI = nullptr;
}

void UCaterUIManager::Construct(UCaterGameInstance* InGameInstance)
{
	GI = InGameInstance;
}

void UCaterUIManager::EndCurrentUIState()
{
	if (!IsValid(GI))
	{
		UE_LOG(LogOnline, Error, TEXT("UCaterUIManager::EndCurrentUIState: GI is null"));
		return;
	}
	switch (CurrentUIState)
	{
	case ECaterUIState::NoMenu:
		break;
	case ECaterUIState::MainMenu:
		DestructMainMenu();
		break;
	case ECaterUIState::InGameMenu:
		DestructInGameMenu();
		break;
	case ECaterUIState::MessageMenu:
		DestructMessageMenu();
		break;
	case ECaterUIState::EndMatchMenu:
		DestructEndMatchMenu();
		break;
	default: ;
	}
}

void UCaterUIManager::SetUIState(ECaterUIState NewState)
{
	if (!IsValid(GI))
	{
		UE_LOG(LogTemp, Error, TEXT("UCaterUIManager::EndCurrentUIState: GI is null"));
		return;
	}
	if(NewState == CurrentUIState) return;
	UE_LOG(LogTemp, Warning, TEXT("Switching UCaterUIManager::SetUIState: %d"), (int)NewState);
	EndCurrentUIState();
	CurrentUIState = NewState;
	switch (NewState)
	{
	case ECaterUIState::NoMenu:
		break;
	case ECaterUIState::MainMenu:
		CreateMainMenu(GI->GetFirstGamePlayer());
		break;
	case ECaterUIState::InGameMenu:
		CreateInGameMenu(GI->GetFirstGamePlayer());
		break;
	case ECaterUIState::MessageMenu:
		CreateMessageMenu(GI->GetFirstGamePlayer());
		break;
	case ECaterUIState::EndMatchMenu:
		CreateEndMatchMenu();
		break;
	}
	
}
