// Fill out your copyright notice in the Description page of Project Settings.


#include "AsteroidActor.h"


// Sets default values
AAsteroidActor::AAsteroidActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AAsteroidActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAsteroidActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

