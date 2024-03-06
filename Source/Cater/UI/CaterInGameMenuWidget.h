#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaterInGameMenuWidget.generated.h"

class UCaterGameInstance;

UCLASS()
class UCaterInGameMenuWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY()
	UCaterGameInstance* GameInstance;
	UPROPERTY()
	ULocalPlayer* PlayerOwner;
public:
	void Construct(UCaterGameInstance* CaterGameInstance, ULocalPlayer* LocalPlayer);

	UFUNCTION(BlueprintCallable, Category = "CppUIFunctions")
	void OnClickServePauseGame();

	UFUNCTION(BlueprintCallable, Category = "CppUIFunctions")
	void OnClickServerResumeGame();

	UFUNCTION(BlueprintCallable, Category = "CppUIFunctions")
	void OnClickCloseMenu();

	UFUNCTION(BlueprintCallable, Category = "CppUIFunctions")
	void OnClickMainMenu();
	
	void ToggleMenu();
};
