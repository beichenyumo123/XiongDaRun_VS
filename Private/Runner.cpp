// Fill out your copyright notice in the Description page of Project Settings.


#include "Runner.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ARunner::ARunner()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// 默认参数初始化
	CurrentLane = 1;         // 默认在中间轨道
	LaneWidth = 270.0f;      // 假设轨道间距是 400 厘米，根据你的跑道调整
	SwitchLaneInterpSpeed = 15.0f; // 插值速度，越大变道越快
	TargetY = 0.0f;
	ForwardRunSpeed = 1.0f; // 默认给满输入值

	// --- 1. 配置弹簧臂 (Spring Arm) ---
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // 摄像机距离角色的距离
	CameraBoom->bUsePawnControlRotation = false; // 跑酷游戏不需要鼠标控制视角
	CameraBoom->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f)); // 让摄像机稍微向下倾斜看角色

	// --- 2. 配置摄像机 (Camera) ---
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // 挂载到弹簧臂的末端
	FollowCamera->bUsePawnControlRotation = false; // 摄像机自身不跟随控制器旋转

	// --- 3. 配置角色移动细节 (优化跑酷手感) ---
	// 确保角色永远朝向运动方向
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 800.0f, 0.0f);
	// 防止角色因为控制器(比如鼠标)意外转动
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

// Called when the game starts or when spawned
void ARunner::BeginPlay()
{
	Super::BeginPlay();
	TargetY = GetActorLocation().Y;
}

// Called every frame
void ARunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) {
		return;
	}

	// --- 自动向前奔跑逻辑 ---
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), ForwardRunSpeed);
	
	// --- 平滑变道逻辑 ---
	FVector CurrentLocation = GetActorLocation();

	// 使用 FMath::FInterpTo 让当前的 Y 坐标平滑过渡到 TargetY
	float NewY = FMath::FInterpTo(CurrentLocation.Y, TargetY, DeltaTime, SwitchLaneInterpSpeed);

	// 更新角色位置。bSweep=true 意味着如果变道途中撞到墙会被挡住
	SetActorLocation(FVector(CurrentLocation.X, NewY, CurrentLocation.Z), true);
}

// Called to bind functionality to input
void ARunner::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
void ARunner::MoveLeft()
{
	// 如果当前轨道大于0（即在中间或右边），允许向左变道
	if (CurrentLane > 0)
	{
		CurrentLane--;
		// 计算新的目标 Y 坐标：中间轨道的基准 Y 减去一个轨道宽度
		TargetY -= LaneWidth;
	}
}
void ARunner::MoveRight()
{
	// 如果当前轨道小于2（即在左边或中间），允许向右变道
	if (CurrentLane < 2)
	{
		CurrentLane++;
		// 计算新的目标 Y 坐标：中间轨道的基准 Y 加上一个轨道宽度
		TargetY += LaneWidth;
	}
}

void ARunner::AddCoin()
{
	CoinCount++;
	// 可选：在左上角打印调试信息，方便我们测试
	UE_LOG(LogTemp, Warning, TEXT("Coins Collected: %d"), CoinCount);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Coins: %d"), CoinCount));
	}
}


void ARunner::Die()
{
	if (bIsDead) return; // 防止重复死亡触发

	bIsDead = true;

	// 停止所有移动
	GetCharacterMovement()->DisableMovement();

	// 调试信息：在屏幕上打印死亡提示
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("WASTED! You hit an obstacle."));
	}*/
	// 触发蓝图事件
	OnPlayerDiedBP();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("WASTED! You hit an obstacle."));
	}
}
