// Fill out your copyright notice in the Description page of Project Settings.


#include "Runner.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Coin.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Curves/CurveFloat.h" // 难度曲线支持

// Sets default values
ARunner::ARunner()
{
	// ���� Tick ��ѯ������ƽ�������ٶȡ�FOV����ɽʱ��������
	PrimaryActorTick.bCanEverTick = true;

	// Ĭ�ϲ�����ʼ��
	CurrentLane = 1;         // Ĭ�����м���
	LaneWidth = 270.0f;      // ������Ȼ�׼ֵ
	SwitchLaneInterpSpeed = 15.0f; // ��ʼ��ֵ�ٶ�
	InitialSwitchLaneInterpSpeed = SwitchLaneInterpSpeed;
	TargetY = 0.0f;
	ForwardRunSpeed = 1.0f; // Ĭ����������

	// --- 1. ���õ��ɱ� (Spring Arm) ---
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // ����������ɫ�ľ���
	CameraBoom->bUsePawnControlRotation = false; // �ܿ���Ϸ����Ҫ�������ӽ�
	CameraBoom->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f)); // �������΢���¸���

	// --- 2. ��������� (Camera) ---
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // ���ص����ɱ۵�ĩ��
	FollowCamera->bUsePawnControlRotation = false;

	// --- 3. ���ý�ɫ�ƶ�ϸ�� ---
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 800.0f, 0.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// --- 4. ���ô�������� ---
	MagnetSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetSphere"));
	MagnetSphere->SetupAttachment(RootComponent);
	MagnetSphere->SetSphereRadius(MagnetRadius);

	// �����Ż�����ʼʱ�������������κ�������ײ�ͼ���¼�����ֹ����Ҫ���ص�ɨ��
	MagnetSphere->SetCollisionProfileName(TEXT("NoCollision"));
	MagnetSphere->SetGenerateOverlapEvents(false);

	// --- 5. ��̬�Ѷ��뷴��������ʼ�� ---
	MinSpeedLimit = 700.0f;
	MaxSpeedLimit = 1800.0f;
	SpeedAccelerationRate = 18.0f; // ÿ��� 18 ���׵��ٶ�
	CurrentMaxWalkSpeed = MinSpeedLimit;

	BaseFOV = 90.0f;
	MaxSpeedFOV = 110.0f;

	ScorePerMeter = 10;
	ScorePerCoinUnit = 100;
	AccumulativeCoinScore = 0;

	// --- 6. ���㡢�������ģʽ��ʼ�� ---
	CameraLeanSensitivity = 0.025f;
	MaxCameraLeanRoll = 6.0f;
	CameraLeanInterpSpeed = 8.0f;

	CurrentComboCount = 0;
	ComboValidWindow = 2.0f;

	bIsFeverModeActive = false;
	FeverModeDuration = 5.0f;
	FeverComboThreshold = 10;
}

// Called when the game starts or when spawned
void ARunner::BeginPlay()
{
	Super::BeginPlay();
	TargetY = GetActorLocation().Y;
	StartX = GetActorLocation().X; // ������ʼ�����㣬���ڼ����ܿ����

	// �趨��ʼ�ƶ��������ٶ�
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MinSpeedLimit;
	}

	// �󶨴�������ص��ص�
	if (MagnetSphere)
	{
		MagnetSphere->OnComponentBeginOverlap.AddDynamic(this, &ARunner::OnMagnetSphereOverlap);
	}

	// =========================================================================
	// 背景山脉搜索逻辑 - 修复Bug并优化性能
	// 1. 优先通过标签搜索，找不到再按名称搜索
	// 2. 所有找到的Actor都会被正确赋值和设置Mobility
	// 3. 缓存根组件指针避免每帧重复获取
	// =========================================================================
	BackgroundActors.Empty();
	CachedBackgroundRootComponents.Empty();
	TArray<AActor*> FoundActors;

	// 1. 优先搜索带有 "Background" 标签的 Actor（最高优先级）
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Background"), FoundActors);

	// 2. 如果标签未找到，再通过名称/网格名称搜索 StaticMeshActor
	if (FoundActors.Num() == 0)
	{
		for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
		{
			AStaticMeshActor* SMActor = *It;
			if (!SMActor) continue;

#if WITH_EDITOR
			FString CheckedActorLabel = SMActor->GetActorLabel();
			if (CheckedActorLabel.Contains(TEXT("Mountain")) || CheckedActorLabel.Contains(TEXT("Background")))
			{
				FoundActors.Add(SMActor);
				continue;
			}
#endif

			FString ActorName = SMActor->GetName();
			if (ActorName.Contains(TEXT("Mountain")) || ActorName.Contains(TEXT("Background")))
			{
				FoundActors.Add(SMActor);
				continue;
			}

			if (SMActor->GetStaticMeshComponent())
			{
				UStaticMesh* StaticMeshAsset = SMActor->GetStaticMeshComponent()->GetStaticMesh();
				if (StaticMeshAsset)
				{
					FString MeshName = StaticMeshAsset->GetName();
					if (MeshName.Contains(TEXT("Mountain")) || MeshName.Contains(TEXT("Background")))
					{
						FoundActors.Add(SMActor);
					}
				}
			}
		}
	}

	// 3. 统一赋值并设置Mobility（修复Bug：之前只有标签搜索为空时才执行）
	BackgroundActors = FoundActors;

	for (AActor* BgActor : BackgroundActors)
	{
		if (BgActor && BgActor->GetRootComponent())
		{
			BgActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			// 缓存根组件指针，避免Tick中每帧重复获取
			CachedBackgroundRootComponents.Add(BgActor->GetRootComponent());
		}
	}

}

// Called every frame
void ARunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) {
		return;
	}

	// =========================================================================
	// �����Ļ��ƣ���̬�Ѷ��ٶ����ߡ�
	// =========================================================================
	// --- 速度更新：支持线性加速度 或 难度曲线 ---
	if (bUseDifficultyCurve && DifficultyCurve)
	{
		// 根据评估模式选择输入值
		float CurveInput = 0.0f;
		if (SpeedCurveMode == ESpeedCurveMode::Distance)
		{
			// 基于距离（米）评估曲线
			CurveInput = DistanceMeters * CurveTimeScale + CurveTimeOffset;
		}
		else
		{
			// 基于时间（秒）评估曲线
			CurveInput = GetWorld()->GetTimeSeconds() * CurveTimeScale + CurveTimeOffset;
		}

		float SpeedMultiplier = DifficultyCurve->GetFloatValue(CurveInput);
		SpeedMultiplier = FMath::Clamp(SpeedMultiplier, 0.0f, 1.0f);

		// 根据曲线值计算目标速度
		float TargetSpeed = FMath::Lerp(MinSpeedLimit, MaxSpeedLimit, SpeedMultiplier);

		// 平滑插值到目标速度，避免突变
		CurrentMaxWalkSpeed = FMath::FInterpTo(CurrentMaxWalkSpeed, TargetSpeed, DeltaTime, CurveInterpSpeed);
	}
	else
	{
		// 使用线性加速度（原有逻辑）
		CurrentMaxWalkSpeed = FMath::Min(CurrentMaxWalkSpeed + SpeedAccelerationRate * DeltaTime, MaxSpeedLimit);
	}

	// --- 狂暴模式速度爆发 ---
	float EffectiveSpeed = CurrentMaxWalkSpeed;
	if (bIsFeverModeActive)
	{
		EffectiveSpeed = CurrentMaxWalkSpeed * FeverSpeedBoostMultiplier;
	}
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = EffectiveSpeed;
	}

	// =========================================================================
	// ���Ӿ����������ٶ������ FOV �����Ч��
	// =========================================================================
	float SpeedRatio = (CurrentMaxWalkSpeed - MinSpeedLimit) / (MaxSpeedLimit - MinSpeedLimit);
	SpeedRatio = FMath::Clamp(SpeedRatio, 0.0f, 1.0f);

	float TargetFOV = FMath::Lerp(BaseFOV, MaxSpeedFOV, SpeedRatio);

	// --- 狂暴模式FOV爆发 ---
	if (bIsFeverModeActive)
	{
		TargetFOV = FeverFOVBoost;
	}

	if (FollowCamera)
	{
		// Fever模式下更快的FOV跟随速度，激活瞬间有爆发感
		float FOVInterpSpeed = bIsFeverModeActive ? 8.0f : 2.0f;
		FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, FOVInterpSpeed);
	}

	// =========================================================================
	// ���ָ��Ż������ٱ������Ӧ��
	// =========================================================================
	float DynamicInterpSpeed = InitialSwitchLaneInterpSpeed * (CurrentMaxWalkSpeed / MinSpeedLimit);
	DynamicInterpSpeed = FMath::Clamp(DynamicInterpSpeed, InitialSwitchLaneInterpSpeed, InitialSwitchLaneInterpSpeed * 1.5f);

	// --- �Զ���ǰ�����߼� ---
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), ForwardRunSpeed);

	// --- ƽ���������������߼� ---
	FVector CurrentLocation = GetActorLocation();

	float NewY = FMath::FInterpTo(CurrentLocation.Y, TargetY, DeltaTime, DynamicInterpSpeed);
	SetActorLocation(FVector(CurrentLocation.X, NewY, CurrentLocation.Z), true);

	// =========================================================================
	// ����������һ�����������㣨Camera Lean����
	// ��������λ�Ʋ�Խ����� Roll ��ƫת�Ƕ�Խ��λ��ƽϢ�� Roll �ָ���
	// =========================================================================
	if (CameraBoom)
	{
		float YDifference = TargetY - CurrentLocation.Y;
		// ƫת��ʽ
		float DesiredRoll = -YDifference * CameraLeanSensitivity;
		DesiredRoll = FMath::Clamp(DesiredRoll, -MaxCameraLeanRoll, MaxCameraLeanRoll);

		FRotator CurrentBoomRot = CameraBoom->GetRelativeRotation();
		float SmoothRoll = FMath::FInterpTo(CurrentBoomRot.Roll, DesiredRoll, DeltaTime, CameraLeanInterpSpeed);

		// ������ Roll������ԭ�е� Pitch
		CameraBoom->SetRelativeRotation(FRotator(CurrentBoomRot.Pitch, CurrentBoomRot.Yaw, SmoothRoll));
	}

	// =========================================================================
	// �����ݽ��㣺�ܿ����ʵʱ���㡿
	// =========================================================================
	float DistanceDelta = CurrentLocation.X - StartX;
	DistanceMeters = FMath::Max(0, FMath::RoundToInt(DistanceDelta / 100.0f));

	// =========================================================================
	// 背景视差优化 - 使用缓存的组件指针，避免每帧重复获取RootComponent
	// =========================================================================
	for (USceneComponent* RootComp : CachedBackgroundRootComponents)
	{
		if (RootComp)
		{
			FVector CurrentBgLoc = RootComp->GetComponentLocation();
			CurrentBgLoc.X = CurrentLocation.X;
			RootComp->SetWorldLocation(CurrentBgLoc, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	// --- 狂暴模式持续震动（用正弦波做脉冲节奏） ---
	if (bIsFeverModeActive && CoinCollectShakeClass)
	{
		float ShakeTime = GetWorld()->GetTimeSeconds() * 3.0f;
		float ShakePulse = (FMath::Sin(ShakeTime) * 0.5f + 0.5f); // 0~1 之间脉冲
		float FinalShakeScale = FeverContinuousShakeScale * (0.6f + ShakePulse * 0.4f);
		PlayCameraShake(CoinCollectShakeClass, FinalShakeScale);
	}
}

// Called to bind functionality to input
void ARunner::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ARunner::MoveLeft()
{
	if (bIsDead) return;

	if (CurrentLane > 0)
	{
		CurrentLane--;
		TargetY -= LaneWidth;
	}
}

void ARunner::MoveRight()
{
	if (bIsDead) return;

	if (CurrentLane < 2)
	{
		CurrentLane++;
		TargetY += LaneWidth;
	}
}

// =========================================================================
// �������������Combo ������� Fever ������ϵ��
// =========================================================================
void ARunner::AddCoin(int32 Amount)
{
	if (bIsDead) return;

	CoinCount += Amount;

	// 1. ��������
	CurrentComboCount++;

	// ����/���� 2.0s ����˥�߼�ʱ��
	GetWorldTimerManager().ClearTimer(ComboResetTimerHandle);
	GetWorldTimerManager().SetTimer(ComboResetTimerHandle, this, &ARunner::ResetCombo, ComboValidWindow, false);

	// 2. �������������������Խ�ߣ�������ҵ÷�Խ���ǣ��������ٳ� 3 ����
	int32 ComboMultiplier = 1;
	if (CurrentComboCount >= 10) ComboMultiplier = 3;
	else if (CurrentComboCount >= 5) ComboMultiplier = 2;

	// 3. �񱩳���
	int32 FeverMultiplier = bIsFeverModeActive ? 2 : 1;

	// �ۼ��ܷ�
	int32 CoinEarnedScore = Amount * ScorePerCoinUnit * ComboMultiplier * FeverMultiplier;
	AccumulativeCoinScore += CoinEarnedScore;

	// �㲥 Combo ״̬����ͼ�����ڵ��� UI ��̬ Combo ͼ����
	OnComboUpdatedBP(CurrentComboCount);

	// 4. �ж��Ƿ񼤻�� Fever Mode
	if (!bIsFeverModeActive && CurrentComboCount >= FeverComboThreshold)
	{
		bIsFeverModeActive = true;

		// �����񱩴��е���ʱ
		GetWorldTimerManager().SetTimer(FeverDurationTimerHandle, this, &ARunner::DeactivateFeverMode, FeverModeDuration, false);

		// ����Ȩһ���Զ�ǿ�����ŵ��Ȧ�����׼���ܴ�Χ������
		if (MagnetSphere)
		{
			MagnetSphere->SetSphereRadius(1500.0f); // ��Χ������ 1500
			MagnetSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
			MagnetSphere->SetGenerateOverlapEvents(true);

			// ˲�佫����Χ���Ѵ��ڵ����н��˲��ȫ������������
			TArray<AActor*> OverlappingActors;
			MagnetSphere->GetOverlappingActors(OverlappingActors, ACoin::StaticClass());
			for (AActor* Actor : OverlappingActors)
			{
				ACoin* OverlappedCoin = Cast<ACoin>(Actor);
				if (OverlappedCoin)
				{
					OverlappedCoin->AttractTo(this);
				}
			}
		}

		// 狂暴激活瞬间强震
		PlayCameraShake(DeathShakeClass, FeverActivationShakeScale);

		// �㲥����ͼ�¼��������Ⱦ����ȫ���������ӡ����ٺ����ȣ�
		OnFeverModeActivatedBP();
	}

	// ���ųԽ�ҵ���ȷ�����
	PlayCameraShake(CoinCollectShakeClass, 0.6f);
}

void ARunner::ResetCombo()
{
	CurrentComboCount = 0;
	OnComboResetBP(); // ֪ͨ��ͼ
}

void ARunner::DeactivateFeverMode()
{
	bIsFeverModeActive = false;

	// �ָ�����趨
	if (!bIsMagnetActive)
	{
		// ������������Ѿ������ˣ��رմ������Ȧ
		if (MagnetSphere)
		{
			MagnetSphere->SetCollisionProfileName(TEXT("NoCollision"));
			MagnetSphere->SetGenerateOverlapEvents(false);
		}
	}
	else
	{
		// �����������ʱЧ�ڣ������ر�׼������Χ
		if (MagnetSphere)
		{
			MagnetSphere->SetSphereRadius(MagnetRadius);
		}
	}

	OnFeverModeDeactivatedBP(); // ֪ͨ��ͼ��ЧϨ��

}

void ARunner::Die()
{
	if (bIsDead) return;

	bIsDead = true;

	GetCharacterMovement()->DisableMovement();

	// ��������ʱ��
	GetWorldTimerManager().ClearTimer(MagnetTimerHandle);
	GetWorldTimerManager().ClearTimer(ComboResetTimerHandle);
	GetWorldTimerManager().ClearTimer(FeverDurationTimerHandle);

	PlayCameraShake(DeathShakeClass, 1.5f);

	OnPlayerDiedBP();

}

// �޸ģ��ۺ��ܵ÷ּ��㹫ʽ������(����÷� + �����ۼӽ�ҵ÷�)
int32 ARunner::GetTotalScore() const
{
	return (DistanceMeters * ScorePerMeter) + AccumulativeCoinScore;
}

void ARunner::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (ShakeClass)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
		}
	}
}

void ARunner::ActivateMagnet()
{
	if (bIsDead) return;

	bIsMagnetActive = true;

	GetWorldTimerManager().SetTimer(MagnetTimerHandle, this, &ARunner::DeactivateMagnet, MagnetDuration, false);

	// ���ڷǿ�ģʽ�²�����뾶����Ϊ�񱩳���������Χ����
	if (MagnetSphere && !bIsFeverModeActive)
	{
		MagnetSphere->SetSphereRadius(MagnetRadius);
		MagnetSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		MagnetSphere->SetGenerateOverlapEvents(true);

		TArray<AActor*> OverlappingActors;
		MagnetSphere->GetOverlappingActors(OverlappingActors, ACoin::StaticClass());
		for (AActor* Actor : OverlappingActors)
		{
			ACoin* OverlappedCoin = Cast<ACoin>(Actor);
			if (OverlappedCoin)
			{
				OverlappedCoin->AttractTo(this);
			}
		}
	}

}

void ARunner::DeactivateMagnet()
{
	bIsMagnetActive = false;

	// ����񱩻�û��������Ҫ�رռ��Ȧ
	if (!bIsFeverModeActive && MagnetSphere)
	{
		MagnetSphere->SetCollisionProfileName(TEXT("NoCollision"));
		MagnetSphere->SetGenerateOverlapEvents(false);
	}

}

void ARunner::OnMagnetSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// ��������������ڿ�״̬�����ص�Ŀ��Ϊ��ң�����������
	if ((bIsMagnetActive || bIsFeverModeActive) && OtherActor && OtherActor->IsA(ACoin::StaticClass()))
	{
		ACoin* Coin = Cast<ACoin>(OtherActor);
		if (Coin)
		{
			Coin->AttractTo(this);
		}
	}
}