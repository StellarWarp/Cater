// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FloatingBox.generated.h"

UCLASS()
class AFloatingBox : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFloatingBox();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Floating Box")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Floating Box")
	int MaxHealth;

	UPROPERTY(EditAnywhere, Category = "Floating Box")
	int Health;

	UPROPERTY(Replicated, EditAnywhere, Category = "Floating Box")
	FTransform FixedTransfrom;

	UPROPERTY(EditAnywhere, Category = "Floating Box")
	bool bActivated;

	UPROPERTY(EditAnywhere, Category = "Floating Box")
	bool bActivatedInit = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool bIsMovable;
	bool bActivating;
	bool bDeactivating;
	float DeactivationAccel;


	void DeactivatedInit();

public:
	UFUNCTION(BlueprintCallable, Category = "Floating Box")
	void SetBoxMovable(bool movable);

	void LocalSetBoxMovable(bool movable);

	UFUNCTION(BlueprintCallable, Category = "Floating Box")
	void SetActivated(bool activated);
	// UFUNCTION(NetMulticast, Unreliable)
	// void ClientSetActivated(bool activated);

	void OnActivatingEnd();
	void OnDeactivatingEnd();
	void SetDeactivatedLocation();
};
