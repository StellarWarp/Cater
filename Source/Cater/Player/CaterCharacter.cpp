// Fill out your copyright notice in the Description page of Project Settings.


#include "CaterCharacter.h"

#include "CaterGameMode.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"

ACaterCharacter::ACaterCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	// GetCharacterMovement()->bConstrainToPlane = true;
	// GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create a decal in the world to show the cursor's location
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/TopDownCPP/Blueprints/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

}
// Called to bind functionality to input
void ACaterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}


void ACaterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CursorToWorld != nullptr)
	{
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UWorld* World = GetWorld())
			{
				FHitResult HitResult;
				FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
				FVector StartLocation = TopDownCameraComponent->GetComponentLocation();
				FVector EndLocation = TopDownCameraComponent->GetComponentRotation().Vector() * 2000.0f;
				Params.AddIgnoredActor(this);
				World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
				FQuat SurfaceRotation = HitResult.ImpactNormal.ToOrientationRotator().Quaternion();
				CursorToWorld->SetWorldLocationAndRotation(HitResult.Location, SurfaceRotation);
			}
		}
		else if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult TraceHitResult;
			PC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}
}


void ACaterCharacter::KilledBy(APawn* EventInstigator)
{
	if (GetLocalRole() == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}




bool ACaterCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<ACaterGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<ACaterCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}

bool ACaterCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if (bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| GetLocalRole() != ROLE_Authority				// not authority
		|| GetWorld()->GetAuthGameMode<ACaterGameMode>() == NULL
		|| GetWorld()->GetAuthGameMode<ACaterGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}




void ACaterCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}
	
	SetReplicatingMovement(false);
	TearOff();
	bIsDying = true;

	// if (GetLocalRole() == ROLE_Authority)
	// {
	// 	ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);
	//
	// 	// play the force feedback effect on the client player controller
	// 	ACaterPlayerController* PC = Cast<ACaterPlayerController>(Controller);
	// 	if (PC && DamageEvent.DamageTypeClass)
	// 	{
	// 		UCaterDamageType *DamageType = Cast<UCaterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
	// 		if (DamageType && DamageType->KilledForceFeedback && PC->IsVibrationEnabled())
	// 		{
	// 			FForceFeedbackParameters FFParams;
	// 			FFParams.Tag = "Damage";
	// 			PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, FFParams);
	// 		}
	// 	}
	// }
	//
	// // cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	// if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
	// {
	// 	UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	// }
	//
	// // remove all weapons
	// DestroyInventory();
	//
	// // switch back to 3rd person view
	// UpdatePawnMeshes();
	//
	// DetachFromControllerPendingDestroy();
	// StopAllAnimMontages();
	//
	// if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	// {
	// 	LowHealthWarningPlayer->Stop();
	// }
	//
	// if (RunLoopAC)
	// {
	// 	RunLoopAC->Stop();
	// }
	//
	// if (GetMesh())
	// {
	// 	static FName CollisionProfileName(TEXT("Ragdoll"));
	// 	GetMesh()->SetCollisionProfileName(CollisionProfileName);
	// }
	// SetActorEnableCollision(true);
	//
	// // Death anim
	// float DeathAnimDuration = PlayAnimMontage(DeathAnim);
	//
	// // Ragdoll
	// if (DeathAnimDuration > 0.f)
	// {
	// 	// Trigger ragdoll a little before the animation early so the character doesn't
	// 	// blend back to its normal position.
	// 	const float TriggerRagdollTime = DeathAnimDuration - 0.7f;
	//
	// 	// Enable blend physics so the bones are properly blending against the montage.
	// 	GetMesh()->bBlendPhysics = true;
	//
	// 	// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
	// 	FTimerHandle TimerHandle;
	// 	GetWorldTimerManager().SetTimer(TimerHandle, this, &ACaterCharacter::SetRagdollPhysics, FMath::Max(0.1f, TriggerRagdollTime), false);
	// }
	// else
	// {
	// 	SetRagdollPhysics();
	// }
	//
	// // disable collisions on capsule
	// GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}





