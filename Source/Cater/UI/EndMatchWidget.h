// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndMatchWidget.generated.h"

class UCaterGameInstance;

UCLASS()
class CATER_API UEndMatchWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY()
	UCaterGameInstance* GI;
	

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndMatchWidget")
	FString EndGameMessage;

	void Construct(UCaterGameInstance* GameInstance);

	UFUNCTION(BlueprintCallable, Category = "EndMatchWidget")
	void OnExitToMainMenuButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "EndMatchWidget")
	void OnRematchButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "EndMatchWidget")
	void OnClickContinueButton();

	
};
