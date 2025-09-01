// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleSpaceship.h"


// Sets default values
ASimpleSpaceship::ASimpleSpaceship()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 컴포넌트 초기화
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetupAttachment(RootComponent);

	// 기본 메시 설정 (원뿔 모양)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		ShipMesh->SetStaticMesh(ConeMesh.Object);
		ShipMesh->SetRelativeScale3D(FVector(0.75f));
		ShipMesh->SetRelativeRotation(FRotator(-90, 0, 0));
	}
	
	// 물리 설정
	ShipMesh->SetEnableGravity(false);
	ShipMesh->SetSimulatePhysics(false);
	ShipMesh->SetCollisionProfileName(TEXT("Pawn"));
	ShipMesh->SetNotifyRigidBodyCollision(true);

	// 플레이어 자동 빙의
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void ASimpleSpaceship::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASimpleSpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ApplyTurnWithInertia(DeltaTime); // 회전 관성
	ApplyForwardMotion(DeltaTime); // 가속도 제한 + 속도 적분
	MoveSimple(DeltaTime); // Speed * Forward로 이동
}

// Called to bind functionality to input
void ASimpleSpaceship::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASimpleSpaceship::Input_MoveForward);
	PlayerInputComponent->BindAxis(TEXT("TurnRight"), this, &ASimpleSpaceship::Input_TurnRight);
}

void ASimpleSpaceship::Input_MoveForward(float Value)
{
	CachedForwardInput = FMath::Clamp(Value, -1.f, 1.f);
}

void ASimpleSpaceship::Input_TurnRight(float Value)
{
	CachedTurnInput = FMath::Clamp(Value, -1.f, 1.f);
}

void ASimpleSpaceship::ApplyForwardMotion(float DeltaTime)
{
	// 전/후 입력으로 “직진 속도”를 바꾸고 키를 떼면 자연스럽게 감속. 급하게 돌면 앞으로 가는 힘이 조금 줄어든다.
	
	// 1) 입력 기반 가속 (최대 가속도 제한)
	const float accelCmd = FMath::Clamp(CachedForwardInput, -1.f, 1.f) * MaxAccel;
	Speed += accelCmd * DeltaTime;

	// 2) 키를 뗐을 때 자연 감속(0을 향해 선형 감소)
	if (FMath::IsNearlyZero(CachedForwardInput) && NoInputDecel > 0.f && !FMath::IsNearlyZero(Speed))
	{
		const float sgn = FMath::Sign(Speed);
		float mag = FMath::Abs(Speed);
		mag = FMath::Max(0.f, mag - NoInputDecel * DeltaTime); // 초당 NoInputDecel 만큼 감속
		Speed = (mag < 1.f ? 0.f : sgn * mag);                 // 아주 작으면 0 스냅
	}

	// 3) 회전 중 속도 손실(각속도 크기에 비례)
	if (TurnDecelPerUnit > 0.f && MaxTurnRateDegPerSec > KINDA_SMALL_NUMBER)
	{
		float speedSign = FMath::Sign(Speed);
		float speedMag  = FMath::Abs(Speed);

		const float turnAlpha = FMath::Clamp(FMath::Abs(AngularVelocityDeg) / MaxTurnRateDegPerSec, 0.f, 1.f);
		const float turnDecel = TurnDecelPerUnit * (turnAlpha * MaxTurnRateDegPerSec); 
		// = TurnDecelPerUnit * |ω|  [uu/s^2]

		speedMag = FMath::Max(0.f, speedMag - turnDecel * DeltaTime);
		Speed = speedMag * speedSign;
	}

	// 4) 속도 상한
	Speed = FMath::Clamp(Speed, -MaxSpeed, +MaxSpeed);

	// 디버그
	if(GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
			*FString::Printf(TEXT("Speed=%.1f"), Speed));
	}
}

void ASimpleSpaceship::ApplyTurnWithInertia(float DeltaTime)
{
	// 좌/우 입력으로 회전 속도를 늘리고, 손을 떼면 감쇠로 서서히 줄이며 그만큼 물체를 돌린다.

	// 1) 입력에 비례한 각가속도
	const float angAccel = FMath::Clamp(CachedTurnInput, -1.f, 1.f) * AngularAccelDegPerSec2;

	// 2) 각속도 적분
	AngularVelocityDeg += angAccel * DeltaTime;

	// 3) 각속도 감쇠(지수) - 관성 유지되며 서서히 감소
	const float k = FMath::Exp(-AngularDampingPerSec * DeltaTime);
	AngularVelocityDeg *= k;

	// 4) 각속도 상한
	AngularVelocityDeg = FMath::Clamp(AngularVelocityDeg, -MaxTurnRateDegPerSec, +MaxTurnRateDegPerSec);

	// 5) 실제 회전 적용
	if (!FMath::IsNearlyZero(AngularVelocityDeg))
	{
		AddActorWorldRotation(FRotator(0.f, AngularVelocityDeg * DeltaTime, 0.f));
	}
}

void ASimpleSpaceship::MoveSimple(float DeltaTime)
{
	// 지금 보고 있는 방향으로 속도 × 시간만큼 단순 이동
	
	if (FMath::IsNearlyZero(Speed)) return;

	const FVector Forward = GetActorForwardVector(); // 현재 앞방향
	const FVector Delta = Forward * (Speed * DeltaTime); // 위치

	// 루트가 USceneComponent이므로 스윕 없이 단순 이동
	AddActorWorldOffset(Delta, false);
}
