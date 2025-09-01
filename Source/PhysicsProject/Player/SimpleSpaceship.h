// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SimpleSpaceship.generated.h"

UCLASS()
class PHYSICSPROJECT_API ASimpleSpaceship : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASimpleSpaceship();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// 입력 함수
	void Input_MoveForward(float Value);
	void Input_TurnRight(float Value);

	// 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ShipMesh;

	// 물리 파라미터
	UPROPERTY(EditAnywhere, Category="Simple")
	float MaxAccel = 2200.f; // 최대 가속도

	UPROPERTY(EditAnywhere, Category="Simple")
	float MaxSpeed = 2200.f; // 최고 속도

	UPROPERTY(EditAnywhere, Category="Simple")
	float NoInputDecel = 1200.f; // 감속량, 값↑: 키 뗄때 빠르게 멈춤

	UPROPERTY(EditAnywhere, Category="Simple")
	float AngularAccelDegPerSec2 = 360.f; // 회전 가속도(좌우속도얼마나 빠르게 커지는지) 값↑: 입력 시 각속도 더 빨리 증가

	UPROPERTY(EditAnywhere, Category="Simple")
	float MaxTurnRateDegPerSec = 180.f; // 각속도(최고 회전 속도), 값↑: 최대 선회 속도 증가

	UPROPERTY(EditAnywhere, Category="Simple")
	float AngularDampingPerSec = 2.0f; // 각속도 지수 감쇠율 (관성 감쇠), 값↑: 회전이 더 빨리 식음(관성 짧아짐)

	UPROPERTY(EditAnywhere, Category="Simple")
	float TurnDecelPerUnit = 8.0f; // 값↑: 회전할때 더 크게 느려짐
	
	float Speed = 0.f; // +전진, -후진 (스칼라 속도만 사용)
	float AngularVelocityDeg = 0.f; // Yaw방향 각속도(관성), 초당 몇도 도는지 (현재 회전 속도)

	float CachedForwardInput = 0.f; // [-1..1]  전/후 입력
	float CachedTurnInput = 0.f; // [-1..1]  좌/우 입력
	
	void ApplyForwardMotion(float DeltaTime); // 가속도 제한 + 속도 적분 + 속도 클램프
	void ApplyTurnWithInertia(float DeltaTime); // 관성 회전(각가속도 적분 + 감쇠 + 클램프)
	void MoveSimple(float DeltaTime); // 현재 바라보는 방향(Forward)으로 Speed × 시간만큼 위치 이동
};
