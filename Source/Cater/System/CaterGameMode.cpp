// Fill out your copyright notice in the Description page of Project Settings.


#include "CaterGameMode.h"

#include "CaterGameInstance.h"
#include "CaterGameSession.h"
#include "CaterGameState.h"
#include "CaterHUD.h"
#include "CaterPlayerController.h"
#include "CaterPlayerState.h"
#include "CaterSpectatorPawn.h"
#include "Misc/CommandLine.h"
#include "UObject/ConstructorHelpers.h"


ACaterGameMode::ACaterGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
		DefaultPawnClass = PlayerPawnBPClass.Class;

	// static ConstructorHelpers::FClassFinder<APawn> BotPawnOb(TEXT("/Game/Blueprints/Pawns/BotPawn"));
	// BotPawnClass = BotPawnOb.Class;

	HUDClass = ACaterHUD::StaticClass();
	PlayerControllerClass = ACaterPlayerController::StaticClass();
	PlayerStateClass = ACaterPlayerState::StaticClass();
	SpectatorClass = ACaterSpectatorPawn::StaticClass();
	GameStateClass = ACaterGameState::StaticClass();
	GameSessionClass = ACaterGameSession::StaticClass();
	// ReplaySpectatorPlayerControllerClass = AShooterDemoSpectator::StaticClass();

	// MinRespawnDelay = 5.0f;
	//
	// bAllowBots = true;	
	// bNeedsBotCreation = true;
	bUseSeamlessTravel = FParse::Param(FCommandLine::Get(), TEXT("NoSeamlessTravel")) ? false : true;
	//if play in editor, do not use seamless travel
	bUseSeamlessTravel = !GIsEditor;
}

void ACaterGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// const int32 BotsCountOptionValue = UGameplayStatics::GetIntOption(Options, GetBotsCountOptionName(), 0);
	// SetAllowBots(BotsCountOptionValue > 0 ? true : false, BotsCountOptionValue);	
	// Super::InitGame(MapName, Options, ErrorMessage);
	//
	// const UGameInstance* GameInstance = GetGameInstance();
	// if (GameInstance && Cast<UCaterGameInstance>(GameInstance)->GetOnlineMode() != EOnlineMode::Offline)
	// {
	// 	bPauseable = false;
	// }
}

void ACaterGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn,
                            const UDamageType* DamageType)
{
	ACaterPlayerState* KillerPlayerState = Killer ? Cast<ACaterPlayerState>(Killer->PlayerState) : NULL;
	ACaterPlayerState* VictimPlayerState = KilledPlayer ? Cast<ACaterPlayerState>(KilledPlayer->PlayerState) : NULL;

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		// KillerPlayerState->ScoreKill(VictimPlayerState, KillScore);
		// KillerPlayerState->InformAboutKill(KillerPlayerState, DamageType, VictimPlayerState);
	}

	if (VictimPlayerState)
	{
		// VictimPlayerState->ScoreDeath(KillerPlayerState, DeathScore);
		// VictimPlayerState->BroadcastDeath(KillerPlayerState, DamageType, VictimPlayerState);
	}
}

FName ACaterGameMode::GetMatchState()
{
	return FName{};
}

void ACaterGameMode::EndMatch()
{
	Super::EndMatch();
}

void ACaterGameMode::OnPlayerFallen(ACaterCharacter* FallenPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("Player %s has fallen"), *FallenPlayer->GetName());

	auto GS = GetGameState<ACaterGameState>();
	GS->bIsGameFinished = true;
	GS->GameEnding = ECaterGameEnding::PlayerFallen;
	//call each player controller to show the end match menu
	for (auto Player : GS->PlayerArray)
	{
		auto PC = Cast<ACaterPlayerController>(Player->GetPawn()->GetController());
		if (PC)
		{
			PC->ClientOnGameFinished();
		}
	}
}

void ACaterGameMode::OnPlayerRespawn(ACaterCharacter* RespawnedPlayer)
{
}

void ACaterGameMode::OnPlayerEnterFinishZone(ACaterCharacter* Player)
{
	auto GS = GetGameState<ACaterGameState>();
	GS->FinishedPlayers.Add(Player);
	if (GS->FinishedPlayers.Num() == GS->PlayerArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("All players finished"));
		GS->bIsGameFinished = true;
		GS->GameEnding = ECaterGameEnding::PlayerFinished;
		// EndMatch();
		for (auto FinPlayer : GS->FinishedPlayers)
		{
			auto PC = Cast<ACaterPlayerController>(FinPlayer->GetController());
			if (PC)
			{
				PC->ClientOnGameFinished();
			}
		}
	}
}

void ACaterGameMode::OnPlayerExitFinishZone(ACaterCharacter* Player)
{
	auto GS = GetGameState<ACaterGameState>();
	GS->FinishedPlayers.Remove(Player);
}
