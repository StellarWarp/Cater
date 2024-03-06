// Fill out your copyright notice in the Description page of Project Settings.


#include "CaterSpectatorPawn.h"


// Sets default values
ACaterSpectatorPawn::ACaterSpectatorPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ACaterSpectatorPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACaterSpectatorPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ACaterSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

