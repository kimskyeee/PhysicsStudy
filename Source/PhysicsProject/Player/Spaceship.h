// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Spaceship.generated.h"

UCLASS()
class PHYSICSPROJECT_API ASpaceship : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASpaceship();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// 입력 함수
	void Input_MoveForward(float Value);
	void Input_TurnRight(float Value);

	// 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ShipMesh;

	// 물리 파라미터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float Acceleration = 2500.0f; // 가속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float MaxSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float LinearDamping = 0.98f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float RotationDeg = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float RotationDamping = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float BounceCoefficient = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float LateralDampingPerSec = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float VelocityTurnDegPerSec = 180.0f;

	// 현재 상태 변수
	FVector CurrentVelocity = FVector::ZeroVector;
	float CurrentAngularVelocity = 0.0f;
	float CachedForwardInput = 0.0f;
	float CachedTurnInput = 0.0f;

	// 물리 함수
	void ApplyAcceleration(float DeltaTime);
	void ApplyRotationWithInertia(float DeltaTime);
	void ApplyDamping();
	void ClampMaxSpeed();
	void MoveWithSweep(float DeltaTime);
	void ReflectVelocityByHit(const FHitResult& Hit);
	void ApplyLateralDamping(float DeltaTime);
	void SteerVelocityTowardsForward(float DeltaTime);
};
