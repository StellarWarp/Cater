#pragma once
#include "CaterGameSession.h"
#include "Blueprint/UserWidget.h"
#include "CaterMainMenuWidget.generated.h"

class UCaterMainMenuWidget;
class ULocalPlayer;
class UCaterGameInstance;

UCLASS(BlueprintType)
class UCaterUISessionListItem :public UObject
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<UCaterMainMenuWidget> MainMenu;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SearchResultsIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MapName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString GameMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CurrentPlayers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MaxPlayers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Ping;

	UFUNCTION(BlueprintCallable, Category = "SessionListItem")
	void OnClickJoinGame() const;
};

UCLASS()
class UCaterMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UCaterGameInstance* GameInstance;
	ULocalPlayer* PlayerOwner;
	UPROPERTY(BlueprintReadOnly, Category = "MainMenu")
	FString StatusText;

	bool bSearchingForServers;

	UPROPERTY()
	TArray<UCaterUISessionListItem*> UIServerList;

public:
	void Construct(UCaterGameInstance* InGameInstance, ULocalPlayer* InPlayerOwner);


	FString GetMapName() const;
	ULocalPlayer* GetPlayerOwner() const;
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;


	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnClickStartGame();

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnClickQuitGame();

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnClickHostGame();
	

	UPROPERTY(BlueprintReadWrite, Category = "MainMenu")
	uint8 bIsDedicatedServer : 1;
	UPROPERTY(BlueprintReadWrite, Category = "MainMenu")
	uint8 bFindLan : 1;

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void FindGame();

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnJoinGame(int32 SessionIndexInSearchResults);



	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnListServer();
	ACaterGameSession* GetGameSession() const;
	void UpdateSearchStatus();
	void OnServerSearchFinished();
	UFUNCTION(BlueprintNativeEvent, Category = "MainMenu")
	void OnUpdateServerMenu(const TArray<UCaterUISessionListItem*>& ServerList);
};
