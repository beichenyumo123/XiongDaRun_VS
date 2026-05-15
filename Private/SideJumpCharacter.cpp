// Fill out your copyright notice in the Description page of Project Settings.


#include "SideJumpCharacter.h"

// Sets default values
ASideJumpCharacter::ASideJumpCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASideJumpCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASideJumpCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASideJumpCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

