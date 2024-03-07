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

	FixedTransfrom = GetActorTransform();

	// if (bActivating || bDeactivating || (bActivated&&!bActivatedInit))
	// {
	// 	if (bDeactivating)
	// 		MeshComponent->SetSimulatePhysics(true);
	// 	if (bActivating || bDeactivating)
	// 		SetBoxMovable(true);
	// 	if (bActivated)
	// 	{
	// 		
	// 	}
	// 	return;
	// }
	// if (GetLocalRole() < ROLE_Authority) return;
	if (bActivatedInit == false)
		DeactivatedInit();
}

// void AFloatingBox::OnRep_bActivated()
// {
// 	// bool IsAtTarget = FMath::IsNearlyEqual(GetActorLocation().Z, FixedTransfrom.GetLocation().Z, 0.1f);
// 	// if (bActivated && !IsAtTarget)
// 	// {
// 	// 	SetBoxMovable(true);
// 	// 	SetActorTransform(FixedTransfrom);
// 	// 	OnActivatingEnd();
// 	// }
// 	// if (!bActivated && IsAtTarget)
// 	// {
// 	// 	DeactivatedInit();
// 	// }
// }


void AFloatingBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFloatingBox, FixedTransfrom);
}

void AFloatingBox::DeactivatedInit()
{
	SetBoxMovable(true);
	bActivated = false;
	FVector InitLocation = GetActorLocation();
	InitLocation.Z -= 50000.0f * (FMath::FRand() + 1);
	SetActorLocation(InitLocation);
	SetBoxMovable(false);
}

void AFloatingBox::SetBoxMovable(bool movable)
{
	bIsMovable = movable;
	MeshComponent->SetMobility(bIsMovable ? EComponentMobility::Movable : EComponentMobility::Static);
}

// void AFloatingBox::OnRep_bIsMovable()
// {
// 	// MeshComponent->SetMobility(bIsMovable ? EComponentMobility::Movable : EComponentMobility::Static);
// }

void AFloatingBox::LocalSetBoxMovable(bool movable)
{
	bIsMovable = movable;
	MeshComponent->SetMobility(bIsMovable ? EComponentMobility::Movable : EComponentMobility::Static);
}

void AFloatingBox::SetActivated(bool activated)
{
	if (bActivated && !activated) // deactivation
	{
		if (bActivating) OnActivatingEnd();
		bDeactivating = true;
		SetBoxMovable(true);
		MeshComponent->SetSimulatePhysics(true);
		DeactivationAccel = (FMath::FRand() + 1) * 1000;
	}
	else if (!bActivated && activated) // activation
	{
		bActivating = true;
		if (bDeactivating) OnDeactivatingEnd();
		SetBoxMovable(true);
	}
	bActivated = activated;
}

// void AFloatingBox::SetActivated(bool activated)
// {
// 	if (GetLocalRole() < ROLE_Authority) return;
// 	ClientSetActivated(activated);
// 	// if (bActivated && !activated) // deactivation
// 	// {
// 	// 	if (bActivating) OnActivatingEnd();
// 	// 	bDeactivating = true;
// 	// 	SetBoxMovable(true);
// 	// 	MeshComponent->SetSimulatePhysics(true);
// 	// 	DeactivationAccel = (FMath::FRand() + 1) * 1000;
// 	// }
// 	// else if (!bActivated && activated) // activation
// 	// {
// 	// 	bActivating = true;
// 	// 	if (bDeactivating) OnDeactivatingEnd();
// 	// 	SetBoxMovable(true);
// 	// }
// 	// bActivated = activated;
// }

// void AFloatingBox::ClientSetActivated_Implementation(bool activated)
// {
// 	if (GetLocalRole() < ROLE_Authority)
// 	{
// 		static int i = 0;
// 		i++;
// 		UE_LOG(LogTemp, Warning, TEXT("---Call on client %d , Object %s"), i, *GetName());
// 	}
// 	else
// 	{
// 		static int i = 0;
// 		i++;
// 		UE_LOG(LogTemp, Warning, TEXT("Call on server %d , Object %s"), i, *GetName());
// 	}
// 	if (bActivated && !activated) // deactivation
// 	{
// 		if (bActivating) OnActivatingEnd();
// 		bDeactivating = true;
// 		SetBoxMovable(true);
// 		MeshComponent->SetSimulatePhysics(true);
// 		DeactivationAccel = (FMath::FRand() + 1) * 1000;
// 	}
// 	else if (!bActivated && activated) // activation
// 	{
// 		bActivating = true;
// 		if (bDeactivating) OnDeactivatingEnd();
// 		SetBoxMovable(true);
// 	}
// 	bActivated = activated;
// }

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
	auto InitLocation = FixedTransfrom.GetLocation();
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
		const float TargetZ = FixedTransfrom.GetLocation().Z;
		float NewZ = FMath::Lerp(Location.Z, TargetZ, DeltaTime * 3);
		Location.Z = NewZ;
		SetActorLocation(Location);
		FQuat CurrentRotation = GetActorRotation().Quaternion();
		FQuat NewRotation = FMath::QInterpTo(CurrentRotation, FixedTransfrom.GetRotation(), DeltaTime, 3.0f);
		SetActorRotation(NewRotation);
		if (FMath::IsNearlyEqual(Location.Z, TargetZ, 0.1f))
		{
			SetActorTransform(FixedTransfrom);
			OnActivatingEnd();
		}
	}

	if (bDeactivating)
	{
		//accelerate up
		MeshComponent->AddForce(FVector(0, 0, DeactivationAccel), NAME_None, true);
		const float TargetZ = FixedTransfrom.GetLocation().Z;
		if (GetActorLocation().Z > TargetZ + 5000)
		{
			SetDeactivatedLocation();
			OnDeactivatingEnd();
		}
	}
}
