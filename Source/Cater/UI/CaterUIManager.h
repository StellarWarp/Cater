// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CaterUIManager.generated.h"

class UEndMatchWidget;
class UCaterGameInstance;
class UCaterMessageMenuWidget;
class UCaterInGameMenuWidget;
class UCaterMainMenuWidget;

UENUM()
enum class ECaterUIState
{
	NoMenu,
	MainMenu,
	InGameMenu,
	MessageMenu,
	EndMatchMenu
};

UCLASS(Blueprintable)
class CATER_API UCaterUIManager : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	UCaterGameInstance* GI;

	UPROPERTY()
	ECaterUIState CurrentUIState;

	UPROPERTY()
	UCaterMainMenuWidget* MainMenuUI;

	UPROPERTY()
	UCaterInGameMenuWidget* InGameMenuUI;

	UPROPERTY()
	UCaterMessageMenuWidget* MessageMenuUI;

	UPROPERTY()
	UEndMatchWidget* EndMatchUI;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCaterMainMenuWidget> MainMenuUIClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCaterInGameMenuWidget> InGameMenuUIClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCaterMessageMenuWidget> MessageMenuUIClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UEndMatchWidget> EndMatchUIClass;

	void CreateMainMenu(ULocalPlayer* Player);
	void DestructMainMenu();
	
	void CreateInGameMenu(ULocalPlayer* Player);
	void DestructInGameMenu();



	void CreateMessageMenu(ULocalPlayer* Player);
	void DestructMessageMenu();

	void CreateEndMatchMenu();
	void DestructEndMatchMenu();
public:
	void Construct(UCaterGameInstance* InGameInstance);
	
	UCaterMainMenuWidget* GetMainMenuUI() const { return MainMenuUI; }
	UCaterInGameMenuWidget* GetInGameMenuUI() const { return InGameMenuUI; }
	UCaterMessageMenuWidget* GetMessageMenuUI() const { return MessageMenuUI; }
	UEndMatchWidget* GetEndMatchUI() const { return EndMatchUI; }

	void EndCurrentUIState();
	void SetUIState(ECaterUIState NewState);

	void ToggleInGameMenu();
	bool IsInGameMenuOpen() const { return InGameMenuUI != nullptr; }
	
};
