// Fill out your copyright notice in the Description page of Project Settings.

#include "SpaceShip.h"

// Sets default values
ASpaceShip::ASpaceShip()
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
void ASpaceShip::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASpaceShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 물리 업데이트 순서
	ApplyRotationWithInertia(DeltaTime);    // 관성이 있는 회전
	ApplyAcceleration(DeltaTime);           // 전방 추력
	ApplyLateralDamping(DeltaTime);         // 옆방향 저항
	SteerVelocityTowardsForward(DeltaTime); // 속도 방향 보정
	ApplyDamping();                         // 전체 선형 감쇠
	ClampMaxSpeed();                        // 속도 제한
	MoveWithSweep(DeltaTime);              // 이동 + 충돌 처리
}

// Called to bind functionality to input
void ASpaceShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASpaceShip::Input_MoveForward);
	PlayerInputComponent->BindAxis(TEXT("TurnRight"), this, &ASpaceShip::Input_TurnRight);
}

void ASpaceShip::Input_MoveForward(float Value)
{
	CachedForwardInput = FMath::Clamp(Value, -1.f, 1.f);
}

void ASpaceShip::Input_TurnRight(float Value)
{
	CachedTurnInput = FMath::Clamp(Value, -1.f, 1.f);
}

void ASpaceShip::ApplyAcceleration(float DeltaTime)
{
	// 추력 가속도를 더해 선속도 적분
	if (FMath::IsNearlyZero(CachedForwardInput)) return;

	const FVector Forward = GetActorForwardVector();
	
	// 점진적 가속도 적용 (부드러운 가속감)
	float AccelMultiplier = FMath::Abs(CachedForwardInput);
	FVector AccelForce = Forward * (Acceleration * CachedForwardInput * AccelMultiplier * DeltaTime);
	
	CurrentVelocity += AccelForce;

	// 속도, 입력 확인
	UE_LOG(LogTemp, Log, TEXT("[Accel] Input=%.2f, Accel=(%.2f, %.2f, %.2f), Vel=(%.2f, %.2f, %.2f)"),
		CachedForwardInput,
		AccelForce.X, AccelForce.Y, AccelForce.Z,
		CurrentVelocity.X, CurrentVelocity.Y, CurrentVelocity.Z);
}

void ASpaceShip::ApplyRotationWithInertia(float DeltaTime)
{
	// 회전 입력이 있을 때 각속도에 가속도 적용
	if (!FMath::IsNearlyZero(CachedTurnInput))
	{
		float RotationAccel = RotationDeg * 2.0f; // 회전 가속도
		CurrentAngularVelocity += CachedTurnInput * RotationAccel * DeltaTime;
	}
	
	// 각속도에 감쇠 적용 (회전 저항)
	const float rotK = FMath::Exp(-RotationDamping * DeltaTime);
	CurrentAngularVelocity *= rotK;
	
	// CurrentAngularVelocity *= RotationDamping;
	
	// 최대 각속도 제한
	float MaxAngularSpeed = RotationDeg * 1.5f;
	CurrentAngularVelocity = FMath::Clamp(CurrentAngularVelocity, -MaxAngularSpeed, MaxAngularSpeed);
	
	// 실제 회전 적용
	if (!FMath::IsNearlyZero(CurrentAngularVelocity))
	{
		FRotator AddYaw = FRotator(0.f, CurrentAngularVelocity * DeltaTime, 0.f);
		AddActorWorldRotation(AddYaw);
	}
}

void ASpaceShip::ApplyDamping()
{
	// 부드러운 선형 감속 (우주 공간의 미세한 저항)
	// CurrentVelocity *= LinearDamping;

	const float linearK = FMath::Exp(-LinearDamping * GetWorld()->GetDeltaSeconds());
	CurrentVelocity *= linearK;
	
	// 아주 작은 속도는 0으로 처리 (떨림 방지)
	if (CurrentVelocity.Length() < 1.0f)
	{
		CurrentVelocity = FVector::ZeroVector;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Damp] |V|=%.2f"), CurrentVelocity.Size());
}

void ASpaceShip::ClampMaxSpeed()
{
	const float Speed = CurrentVelocity.Length();
	if (Speed > MaxSpeed)
	{
		CurrentVelocity = CurrentVelocity.GetSafeNormal() * MaxSpeed;
	}
}

void ASpaceShip::MoveWithSweep(float DeltaTime)
{
	if (CurrentVelocity.IsNearlyZero()) return;

	const FVector Delta = CurrentVelocity * DeltaTime;
	FHitResult Hit;
	AddActorWorldOffset(Delta, true, &Hit);

	if (Hit.IsValidBlockingHit())
	{
		ReflectVelocityByHit(Hit);
	}
}

void ASpaceShip::ReflectVelocityByHit(const FHitResult& Hit)
{
	FVector Nor = Hit.Normal.GetSafeNormal();

	const FVector Reflected = CurrentVelocity - (2.f * FVector::DotProduct(CurrentVelocity, Nor) * Nor);
	CurrentVelocity = Reflected * BounceCoefficient;
	
	// 회전도 충돌에 영향받게 (선택사항)
	CurrentAngularVelocity *= -0.5f; // 충돌시 회전 반대 + 감쇠
	
	// 겹침 방지
	AddActorWorldOffset(Nor * 5.f, false);
}

void ASpaceShip::ApplyLateralDamping(float DeltaTime)
{
	// 속도를 앞방향 성분과 옆방향 성분으로 분해 후 옆방향 성분만 지수감쇠
	if (CurrentVelocity.IsNearlyZero()) return;

	const FVector Fwd = GetActorForwardVector().GetSafeNormal();
	const float vDot = FVector::DotProduct(CurrentVelocity, Fwd);
	const FVector V_parallel = vDot * Fwd;
	FVector V_perp = CurrentVelocity - V_parallel;

	// 옆방향 성분 감쇠
	const float k = FMath::Exp(-LateralDampingPerSec * DeltaTime);
	V_perp *= k;

	CurrentVelocity = V_parallel + V_perp;

	UE_LOG(LogTemp, Warning, TEXT("[LatDamp] |V|=%.2f"), CurrentVelocity.Size());
}

void ASpaceShip::SteerVelocityTowardsForward(float DeltaTime)
{
	// 속도의 방향만 일정 각속도로 앞방향에 맞춰 회전(속력 크기는 유지)
	if (CurrentVelocity.IsNearlyZero() || VelocityTurnDegPerSec <= 0.f) return;

	const float Speed = CurrentVelocity.Length();
	const FVector Dir = CurrentVelocity / Speed;
	const FVector Target = GetActorForwardVector().GetSafeNormal();

	const FVector NewDir = FMath::VInterpNormalRotationTo(Dir, Target, DeltaTime, VelocityTurnDegPerSec);
	CurrentVelocity = NewDir * Speed;

	UE_LOG(LogTemp, Warning, TEXT("[Steer] |V|=%.2f"), CurrentVelocity.Size());
}