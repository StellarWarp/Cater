// Fill out your copyright notice in the Description page of Project Settings.


#include "FloatingBox.h"

#include "Net/UnrealNetwork.h"


// Sets default values
AFloatingBox::AFloatingBox()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	bReplicates = true;
}

// Called when the game starts or when spawned
void AFloatingBox::BeginPlay()
{
	Super::BeginPlay();

	FixedLocation = GetActorLocation();

	if(bActivatedInit == false)
	{
		SetBoxMovable(true);
		bActivated = false;
		FVector InitLocation = GetActorLocation();
		InitLocation.Z -= 50000.0f * (FMath::FRand() + 1);
		SetActorLocation(InitLocation);
		SetBoxMovable(false);
	}
}

void AFloatingBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFloatingBox, FixedLocation);
	DOREPLIFETIME(AFloatingBox, bActivated);
	DOREPLIFETIME(AFloatingBox, bActivatedInit);
	DOREPLIFETIME(AFloatingBox, bIsMovable);
	DOREPLIFETIME(AFloatingBox, bActivating);
	DOREPLIFETIME(AFloatingBox, bDeactivating);
	DOREPLIFETIME(AFloatingBox, DeactivationAccel);
}

void AFloatingBox::SetBoxMovable(bool movable)
{
	bIsMovable = movable;
	MeshComponent->SetMobility(bIsMovable ? EComponentMobility::Movable : EComponentMobility::Static);
}

void AFloatingBox::SetActivated(bool activated)
{
	if(bActivated && !activated) // deactivation
	{
		if (bActivating) OnActivatingEnd();
		bDeactivating = true;
		SetBoxMovable(true);
		MeshComponent->SetSimulatePhysics(true);
		DeactivationAccel = (FMath::FRand() + 1)* 1000;
	}
	else if(!bActivated && activated) // activation
	{
		bActivating = true;
		if (bDeactivating) OnDeactivatingEnd();
		SetBoxMovable(true);
	}
	bActivated = activated;
}

void AFloatingBox::OnActivatingEnd()
{
	SetBoxMovable(false);
	bActivating = false;
}

void AFloatingBox::OnDeactivatingEnd()
{
	MeshComponent->SetSimulatePhysics(false);
	SetBoxMovable(false);
	bDeactivating = false;
}

void AFloatingBox::SetDeactivatedLocation()
{
	auto InitLocation = FixedLocation;
	InitLocation.Z -= 50000.0f * (FMath::FRand() + 1);
	SetActorLocation(InitLocation);
	SetActorRotation(FRotator(0, 0, 0));
}

// Called every frame
void AFloatingBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bActivating)
	{
		auto Location = GetActorLocation();
		float ZDistance = Location.Z - FixedLocation.Z;
		float NewZ = FMath::Lerp(Location.Z, FixedLocation.Z, DeltaTime*3);
		Location.Z = NewZ;
		SetActorLocation(Location);
		if (FMath::IsNearlyEqual(Location.Z, FixedLocation.Z, 0.1f))
		{
			SetActorLocation(FixedLocation);
			OnActivatingEnd();
		}
	}

	if(bDeactivating)
	{
		//accelerate up
		MeshComponent->AddForce(FVector(0, 0, DeactivationAccel), NAME_None, true);
		if (GetActorLocation().Z > FixedLocation.Z + 5000)
		{
			SetDeactivatedLocation();
			OnDeactivatingEnd();
		}
	}

	
}
