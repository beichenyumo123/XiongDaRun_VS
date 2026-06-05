// Fill out your copyright notice in the Description page of Project Settings.


#include "Runner.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraShakeBase.h" // 引入相机抖动头文件，保障编译通过
#include "Coin.h"
#include "Kismet/GameplayStatics.h" // 引入游戏静态方法库
#include "Engine/StaticMeshActor.h" // 引入静态网格体 Actor 头文件
#include "Components/StaticMeshComponent.h" // 引入静态网格体组件头文件
#include "Engine/StaticMesh.h" // 引入静态网格体资产头文件
#include "EngineUtils.h" // 引入高效 Actor 迭代器
#include "TimerManager.h" // 【新增】：引入计时器管理器头文件，解决 GetWorldTimerManager 编译不完整类型报错

// Sets default values
ARunner::ARunner()
{
	// 开启 Tick 轮询，用于平滑更新速度、FOV、大山时差、相机侧倾
	PrimaryActorTick.bCanEverTick = true;

	// 默认参数初始化
	CurrentLane = 1;         // 默认在中间轨道
	LaneWidth = 270.0f;      // 轨道宽度基准值
	SwitchLaneInterpSpeed = 15.0f; // 初始插值速度
	InitialSwitchLaneInterpSpeed = SwitchLaneInterpSpeed;
	TargetY = 0.0f;
	ForwardRunSpeed = 1.0f; // 默认满额输入

	// --- 1. 配置弹簧臂 (Spring Arm) ---
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // 摄像机距离角色的距离
	CameraBoom->bUsePawnControlRotation = false; // 跑酷游戏不需要鼠标控制视角
	CameraBoom->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f)); // 摄像机稍微向下俯视

	// --- 2. 配置摄像机 (Camera) ---
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // 挂载到弹簧臂的末端
	FollowCamera->bUsePawnControlRotation = false;

	// --- 3. 配置角色移动细节 ---
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 800.0f, 0.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// --- 4. 配置磁吸球组件 ---
	MagnetSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetSphere"));
	MagnetSphere->SetupAttachment(RootComponent);
	MagnetSphere->SetSphereRadius(MagnetRadius);

	// 性能优化：初始时，磁吸球不启用任何物理碰撞和检测事件，防止不必要的重叠扫描
	MagnetSphere->SetCollisionProfileName(TEXT("NoCollision"));
	MagnetSphere->SetGenerateOverlapEvents(false);

	// --- 5. 动态难度与反馈参数初始化 ---
	MinSpeedLimit = 700.0f;
	MaxSpeedLimit = 1800.0f;
	SpeedAccelerationRate = 18.0f; // 每秒加 18 厘米的速度
	CurrentMaxWalkSpeed = MinSpeedLimit;

	BaseFOV = 90.0f;
	MaxSpeedFOV = 110.0f;

	ScorePerMeter = 10;
	ScorePerCoinUnit = 100;
	AccumulativeCoinScore = 0;

	// --- 6. 侧倾、连击与狂暴模式初始化 ---
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
	StartX = GetActorLocation().X; // 锁定初始出生点，用于计算跑酷距离

	// 设定初始移动组件最大速度
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MinSpeedLimit;
	}

	// 绑定磁力球的重叠回调
	if (MagnetSphere)
	{
		MagnetSphere->OnComponentBeginOverlap.AddDynamic(this, &ARunner::OnMagnetSphereOverlap);
	}

	// =========================================================================
	// 【快速搜寻大山 - 迭代器优化】：
	// 强行清空数组重新搜寻。
	// =========================================================================
	BackgroundActors.Empty();
	TArray<AActor*> FoundActors;

	// 1. 尝试寻找所有带有 "Background" 标签的 Actor（第一优先级）
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Background"), FoundActors);

	// 2. 标签未果，启动高效的 StaticMeshActor 迭代器深度扫描
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

		BackgroundActors = FoundActors;

		// 动态将大山的移动性设置为 Movable 消除警告
		for (AActor* BgActor : BackgroundActors)
		{
			if (BgActor && BgActor->GetRootComponent())
			{
				BgActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			}
		}

		if (GEngine && BackgroundActors.Num() > 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
				FString::Printf(TEXT("SUCCESS: Auto-linked %d Background Mountains!"), BackgroundActors.Num()));
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
	// 【核心机制：动态难度速度曲线】
	// =========================================================================
	CurrentMaxWalkSpeed = FMath::Min(CurrentMaxWalkSpeed + SpeedAccelerationRate * DeltaTime, MaxSpeedLimit);
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = CurrentMaxWalkSpeed;
	}

	// =========================================================================
	// 【视觉张力：随速度拉伸的 FOV 广角特效】
	// =========================================================================
	float SpeedRatio = (CurrentMaxWalkSpeed - MinSpeedLimit) / (MaxSpeedLimit - MinSpeedLimit);
	SpeedRatio = FMath::Clamp(SpeedRatio, 0.0f, 1.0f);

	float TargetFOV = FMath::Lerp(BaseFOV, MaxSpeedFOV, SpeedRatio);
	if (FollowCamera)
	{
		// 阻尼平滑拉伸
		FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, 2.0f);
	}

	// =========================================================================
	// 【手感优化：高速变道自适应】
	// =========================================================================
	float DynamicInterpSpeed = InitialSwitchLaneInterpSpeed * (CurrentMaxWalkSpeed / MinSpeedLimit);
	DynamicInterpSpeed = FMath::Clamp(DynamicInterpSpeed, InitialSwitchLaneInterpSpeed, InitialSwitchLaneInterpSpeed * 1.5f);

	// --- 自动向前奔跑逻辑 ---
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), ForwardRunSpeed);

	// --- 平滑变道与相机侧倾逻辑 ---
	FVector CurrentLocation = GetActorLocation();

	float NewY = FMath::FInterpTo(CurrentLocation.Y, TargetY, DeltaTime, DynamicInterpSpeed);
	SetActorLocation(FVector(CurrentLocation.X, NewY, CurrentLocation.Z), true);

	// =========================================================================
	// 【核心亮点一：变道相机侧倾（Camera Lean）】
	// 左右离心位移差越大，相机 Roll 的偏转角度越大，位移平息后 Roll 恢复。
	// =========================================================================
	if (CameraBoom)
	{
		float YDifference = TargetY - CurrentLocation.Y;
		// 偏转公式
		float DesiredRoll = -YDifference * CameraLeanSensitivity;
		DesiredRoll = FMath::Clamp(DesiredRoll, -MaxCameraLeanRoll, MaxCameraLeanRoll);

		FRotator CurrentBoomRot = CameraBoom->GetRelativeRotation();
		float SmoothRoll = FMath::FInterpTo(CurrentBoomRot.Roll, DesiredRoll, DeltaTime, CameraLeanInterpSpeed);

		// 仅更新 Roll，保留原有的 Pitch
		CameraBoom->SetRelativeRotation(FRotator(CurrentBoomRot.Pitch, CurrentBoomRot.Yaw, SmoothRoll));
	}

	// =========================================================================
	// 【数据结算：跑酷距离实时折算】
	// =========================================================================
	float DistanceDelta = CurrentLocation.X - StartX;
	DistanceMeters = FMath::Max(0, FMath::RoundToInt(DistanceDelta / 100.0f));

	// =========================================================================
	// 【极致性能优化 - 时差大山移动】：
	// =========================================================================
	for (AActor* BgActor : BackgroundActors)
	{
		if (BgActor)
		{
			USceneComponent* RootComp = BgActor->GetRootComponent();
			if (RootComp)
			{
				FVector CurrentBgLoc = RootComp->GetComponentLocation();
				CurrentBgLoc.X = CurrentLocation.X;

				RootComp->SetWorldLocation(CurrentBgLoc, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

	// =========================================================================
	// 【狂暴模式环境细节反馈】：
	// 在 Fever Mode 期间，相机产生非常轻微且刺激的高频持续颤抖。
	// =========================================================================
	if (bIsFeverModeActive && CoinCollectShakeClass)
	{
		PlayCameraShake(CoinCollectShakeClass, 0.1f);
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
// 【核心亮点二：Combo 连击与狂暴 Fever 结算体系】
// =========================================================================
void ARunner::AddCoin(int32 Amount)
{
	if (bIsDead) return;

	CoinCount += Amount;

	// 1. 连击计算
	CurrentComboCount++;

	// 重置/触发 2.0s 连击衰竭计时器
	GetWorldTimerManager().ClearTimer(ComboResetTimerHandle);
	GetWorldTimerManager().SetTimer(ComboResetTimerHandle, this, &ARunner::ResetCombo, ComboValidWindow, false);

	// 2. 计算分数乘数（连击数越高，单个金币得分越暴涨，满连击再乘 3 ！）
	int32 ComboMultiplier = 1;
	if (CurrentComboCount >= 10) ComboMultiplier = 3;
	else if (CurrentComboCount >= 5) ComboMultiplier = 2;

	// 3. 狂暴乘数
	int32 FeverMultiplier = bIsFeverModeActive ? 2 : 1;

	// 累加总分
	int32 CoinEarnedScore = Amount * ScorePerCoinUnit * ComboMultiplier * FeverMultiplier;
	AccumulativeCoinScore += CoinEarnedScore;

	// 广播 Combo 状态给蓝图（用于弹出 UI 动态 Combo 图表）
	OnComboUpdatedBP(CurrentComboCount);

	// 4. 判定是否激活狂暴 Fever Mode
	if (!bIsFeverModeActive && CurrentComboCount >= FeverComboThreshold)
	{
		bIsFeverModeActive = true;

		// 触发狂暴大招倒计时
		GetWorldTimerManager().SetTimer(FeverDurationTimerHandle, this, &ARunner::DeactivateFeverMode, FeverModeDuration, false);

		// 狂暴特权一：自动强行扩张电磁圈并彻底激活超能大范围磁力！
		if (MagnetSphere)
		{
			MagnetSphere->SetSphereRadius(1500.0f); // 范围翻倍到 1500
			MagnetSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
			MagnetSphere->SetGenerateOverlapEvents(true);

			// 瞬间将超大范围内已存在的所有金币瞬间全数吸附过来！
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

		// 视觉震撼：播放剧烈震撼
		PlayCameraShake(DeathShakeClass, 0.8f);

		// 广播给蓝图事件激活狂暴渲染（如全屏火焰粒子、加速红屏等）
		OnFeverModeActivatedBP();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 5.0f, FColor::Red, TEXT("!!! FEVER MODE ACTIVE! DOUBLE POINTS & SUPER MAGNET !!!"));
		}
	}

	// 播放吃金币的轻度反馈震动
	PlayCameraShake(CoinCollectShakeClass, 0.6f);

	if (GEngine)
	{
		FString DebugStr = FString::Printf(TEXT("Coins: %d (+%d Score) | Combo: %d | Fever: %s"),
			CoinCount, CoinEarnedScore, CurrentComboCount, bIsFeverModeActive ? TEXT("ACTIVE") : TEXT("OFF"));
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, DebugStr);
	}
}

void ARunner::ResetCombo()
{
	CurrentComboCount = 0;
	OnComboResetBP(); // 通知蓝图
}

void ARunner::DeactivateFeverMode()
{
	bIsFeverModeActive = false;

	// 恢复电磁设定
	if (!bIsMagnetActive)
	{
		// 如果磁铁本身已经到期了，关闭磁吸检测圈
		if (MagnetSphere)
		{
			MagnetSphere->SetCollisionProfileName(TEXT("NoCollision"));
			MagnetSphere->SetGenerateOverlapEvents(false);
		}
	}
	else
	{
		// 如果磁铁还在时效内，收缩回标准磁铁范围
		if (MagnetSphere)
		{
			MagnetSphere->SetSphereRadius(MagnetRadius);
		}
	}

	OnFeverModeDeactivatedBP(); // 通知蓝图特效熄灭

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 3.0f, FColor::Silver, TEXT("Fever mode expired."));
	}
}

void ARunner::Die()
{
	if (bIsDead) return;

	bIsDead = true;

	GetCharacterMovement()->DisableMovement();

	// 清理倒计时器
	GetWorldTimerManager().ClearTimer(MagnetTimerHandle);
	GetWorldTimerManager().ClearTimer(ComboResetTimerHandle);
	GetWorldTimerManager().ClearTimer(FeverDurationTimerHandle);

	PlayCameraShake(DeathShakeClass, 1.5f);

	OnPlayerDiedBP();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("WASTED! You hit an obstacle."));
	}
}

// 修改：综合总得分计算公式包含：(距离得分 + 连击累加金币得分)
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

	// 仅在非狂暴模式下才重设半径（因为狂暴超级磁吸范围更大）
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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Cyan, TEXT("MAGNET ACTIVE! Nearby coins will fly to you!"));
	}
}

void ARunner::DeactivateMagnet()
{
	bIsMagnetActive = false;

	// 如果狂暴还没结束，不要关闭检测圈
	if (!bIsFeverModeActive && MagnetSphere)
	{
		MagnetSphere->SetCollisionProfileName(TEXT("NoCollision"));
		MagnetSphere->SetGenerateOverlapEvents(false);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Silver, TEXT("Magnet Expired."));
	}
}

void ARunner::OnMagnetSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 如果磁吸开启或处于狂暴状态，且重叠目标为金币，则命令吸附
	if ((bIsMagnetActive || bIsFeverModeActive) && OtherActor && OtherActor->IsA(ACoin::StaticClass()))
	{
		ACoin* Coin = Cast<ACoin>(OtherActor);
		if (Coin)
		{
			Coin->AttractTo(this);
		}
	}
}