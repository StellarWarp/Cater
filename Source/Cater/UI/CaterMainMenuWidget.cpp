#include "CaterMainMenuWidget.h"

#include "CaterGameInstance.h"
#include "CaterGameSession.h"
#include "OnlineSessionSettings.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"


void UCaterMainMenuWidget::Construct(UCaterGameInstance* InGameInstance, ULocalPlayer* InPlayerOwner)
{
	check(IsValid(InGameInstance));
	check(IsValid(InPlayerOwner));

	GameInstance = InGameInstance;
	PlayerOwner = InPlayerOwner;
}

FString UCaterMainMenuWidget::GetMapName() const
{
	return FString(TEXT("MainMenu"));
}

ULocalPlayer* UCaterMainMenuWidget::GetPlayerOwner() const
{
	return PlayerOwner;
}

void UCaterMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bSearchingForServers)
		UpdateSearchStatus();
}

void UCaterMainMenuWidget::OnClickStartGame()
{
	//log
	UE_LOG(LogTemp, Warning, TEXT("OnStartGame"));

	if (!ensure(IsValid(GameInstance)) || GetPlayerOwner() == NULL) return;
	FString const StartURL = FString::Printf(TEXT("/Game/Maps/%s?game=%s%s"),
											 L"TopDownExampleMap",
											 GameInstance->GetOnlineMode() != EOnlineMode::Offline
												 ? TEXT("?listen")
												 : TEXT(""),
											 GameInstance->GetOnlineMode() == EOnlineMode::LAN
												 ? TEXT("?bIsLanMatch")
												 : TEXT(""));

	// Game instance will handle success, failure and dialogs
	FString const GameType = TEXT("");
	// GameInstance->HostGame(GetPlayerOwner(), GameType, StartURL);
	GetWorld()->ServerTravel(StartURL);

	// UGameplayStatics::OpenLevel(GetWorld(), L"TopDownExampleMap", true, TEXT("listen"));
}


void UCaterMainMenuWidget::OnClickQuitGame()
{
	//log
	UE_LOG(LogTemp, Warning, TEXT("OnQuitGame"));

	GameInstance->GotoState(CaterGameInstanceState::None);

	GetWorld()->GetFirstPlayerController()->ConsoleCommand("quit");
}

void UCaterMainMenuWidget::OnClickHostGame()
{
	if (!ensure(IsValid(GameInstance)) || GetPlayerOwner() == NULL) return;
	FString const StartURL = FString::Printf(TEXT("/Game/Maps/%s?game=%s%s"),
	                                          L"TopDownExampleMap",
	                                         GameInstance->GetOnlineMode() != EOnlineMode::Offline
		                                         ? TEXT("?listen")
		                                         : TEXT(""),
	                                         GameInstance->GetOnlineMode() == EOnlineMode::LAN
		                                         ? TEXT("?bIsLanMatch")
		                                         : TEXT(""));

	// Game instance will handle success, failure and dialogs
	FString const GameType = TEXT("");
	GameInstance->HostGame(GetPlayerOwner(), GameType, StartURL);
}

void UCaterMainMenuWidget::OnListServer()
{
	if (!ensure(IsValid(GameInstance)) || GetPlayerOwner() == NULL) return;

	if (!GameInstance->GetGameSession())
	{
		UE_LOG(LogTemp, Error, TEXT("GameSession is NULL"));
		return;
	}
	GameInstance->FindSessions(GetPlayerOwner(), bIsDedicatedServer, bFindLan);
	bSearchingForServers = true;
}

ACaterGameSession* UCaterMainMenuWidget::GetGameSession() const
{
	if (!ensure(IsValid(GameInstance)) || GetPlayerOwner() == NULL) return nullptr;
	return GameInstance->GetGameSession();
}

void UCaterMainMenuWidget::UpdateSearchStatus()
{
	check(bSearchingForServers); // should not be called otherwise

	bool bFinishSearch = true;
	ACaterGameSession* ShooterSession = GetGameSession();
	if (ShooterSession)
	{
		int32 CurrentSearchIdx, NumSearchResults;
		EOnlineAsyncTaskState::Type SearchState = ShooterSession->GetSearchResultStatus(
			CurrentSearchIdx, NumSearchResults);

		UE_LOG(LogOnlineGame, Log, TEXT("ShooterSession->GetSearchResultStatus: %s"),
		       EOnlineAsyncTaskState::ToString(SearchState));

		switch (SearchState)
		{
		case EOnlineAsyncTaskState::InProgress:
			StatusText = TEXT("Searching");
			bFinishSearch = false;
			break;

		case EOnlineAsyncTaskState::Done:
			// copy the results
			{
				UIServerList.Empty();
				const TArray<FOnlineSessionSearchResult>& SearchResults = ShooterSession->GetSearchResults();
				check(SearchResults.Num() == NumSearchResults);
				if (NumSearchResults == 0)
				{
					StatusText = TEXT("NoServersFound");
				}
				else
				{
					StatusText = TEXT("ServersRefresh");
				}

				for (int32 IdxResult = 0; IdxResult < NumSearchResults; ++IdxResult)
				{
					UCaterUISessionListItem* NewServerEntry = NewObject<UCaterUISessionListItem>(this);
					const FOnlineSessionSearchResult& Result = SearchResults[IdxResult];

					NewServerEntry->MainMenu = this;
					NewServerEntry->ServerName = Result.Session.OwningUserName;
					NewServerEntry->Ping = FString::FromInt(Result.PingInMs);
					NewServerEntry->CurrentPlayers = FString::FromInt(
						Result.Session.SessionSettings.NumPublicConnections
						+ Result.Session.SessionSettings.NumPrivateConnections
						- Result.Session.NumOpenPublicConnections
						- Result.Session.NumOpenPrivateConnections);
					NewServerEntry->MaxPlayers = FString::FromInt(Result.Session.SessionSettings.NumPublicConnections
						+ Result.Session.SessionSettings.NumPrivateConnections);
					NewServerEntry->SearchResultsIndex = IdxResult;

					// Result.Session.SessionSettings.Get(SETTING_GAMEMODE, NewServerEntry->GameType);
					Result.Session.SessionSettings.Get(SETTING_MAPNAME, NewServerEntry->MapName);

					UIServerList.Add(NewServerEntry);
				}
			}
			break;

		case EOnlineAsyncTaskState::Failed:
		// intended fall-through
		case EOnlineAsyncTaskState::NotStarted:
			StatusText = FString();
		// intended fall-through
		default:
			break;
		}
	}

	if (bFinishSearch)
	{
		OnServerSearchFinished();
	}
}

void UCaterMainMenuWidget::OnServerSearchFinished()
{
	bSearchingForServers = false;

	TArray<UCaterUISessionListItem*> ServerListPtrs;
	for (auto Entry : UIServerList)
	{
		ServerListPtrs.Add(Entry);
	}
	OnUpdateServerMenu(ServerListPtrs);
}

void UCaterMainMenuWidget::FindGame()
{
	if (!ensure(IsValid(GameInstance)) || GetPlayerOwner() == NULL) return;

	// GameInstance->FindSessions(GetPlayerOwner(),bIsDedicatedServer, bFindLan);
}

void UCaterMainMenuWidget::OnJoinGame(int32 SessionIndexInSearchResults)
{
	if (!ensure(IsValid(GameInstance)) || GetPlayerOwner() == NULL) return;

	UE_LOG(LogTemp, Warning, TEXT("Joining Session: %d"), SessionIndexInSearchResults);

	GameInstance->JoinSession(GetPlayerOwner(), SessionIndexInSearchResults);
}

void UCaterMainMenuWidget::OnUpdateServerMenu_Implementation(const TArray<UCaterUISessionListItem*>& ServerList)
{
	for (auto& Res : GameInstance->GetGameSession()->GetSearchResults())
	{
		UE_LOG(LogTemp, Warning, TEXT("Session Name: %s"), *Res.Session.OwningUserName);
	}
}

void UCaterUISessionListItem::OnClickJoinGame() const
{
	MainMenu->OnJoinGame(SearchResultsIndex);
}
