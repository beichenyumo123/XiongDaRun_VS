// Fill out your copyright notice in the Description page of Project Settings.


#include "Coin.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runner.h" // 引入主角的头文件
#include "Components/CapsuleComponent.h" // 【新增】：引入胶囊体头文件以进行精准碰撞判断

// Sets default values
ACoin::ACoin()
{
    // =========================================================================
    // 【高级性能优化】：开启 Tick 功能，但是默认【关闭】运行时的 Tick 轮询。
    // 只有当金币被磁铁吸附时，我们才动态开启 Tick，平时对 CPU 消耗为零。
    // =========================================================================
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // 1. 初始化碰撞球，并设为根组件
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->InitSphereRadius(50.0f); // 金币感应范围
    CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    // 2. 初始化网格体
    CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
    CoinMesh->SetupAttachment(RootComponent);
    // 金币网格体本身不需要物理碰撞，只做纯展示
    CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    TargetActor = nullptr;
}

// Called when the game starts or when spawned
void ACoin::BeginPlay()
{
    Super::BeginPlay();
    // 绑定重叠事件
    if (CollisionSphere)
    {
        CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACoin::OnSphereOverlap);
    }
}

// Called every frame
void ACoin::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 如果处于被吸引状态，执行向目标（主角）飞行的逻辑
    if (TargetActor)
    {
        FVector CurrentLocation = GetActorLocation();
        FVector TargetLocation = TargetActor->GetActorLocation();

        // 磁吸体验优化：加速度让飞行极具冲击感
        CurrentFlySpeed += FlyAcceleration * DeltaTime;

        // 使用常量插值平滑移动到玩家
        FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaTime, CurrentFlySpeed);
        SetActorLocation(NewLocation);
    }
}

void ACoin::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 如果碰到的是玩家角色
    if (OtherActor && OtherActor->IsA(ARunner::StaticClass()))
    {
        ARunner* Runner = Cast<ARunner>(OtherActor);
        if (Runner)
        {
            // =========================================================================
            // 【核心修复】：精准碰撞检测
            // 只有当金币碰到了玩家的“身体胶囊体”（Capsule Component），才触发吃金币逻辑。
            // 这样，当外围的 MagnetSphere（磁吸球）碰到金币时，只触发 Runner.cpp 里的 AttractTo()，
            // 绝不会在这里提前销毁金币，从而保留了平滑飞向主角的动画！
            // =========================================================================
            if (OtherComp == Runner->GetCapsuleComponent())
            {
                // 1. 玩家数据层：金币+1
                Runner->AddCoin();

                // 2. 表现层：通知蓝图去播放声音和特效
                OnCoinCollectedBP();

                // 3. 销毁金币自身
                Destroy();
            }
        }
    }
}

void ACoin::AttractTo(AActor* Player)
{
    if (Player && !TargetActor)
    {
        TargetActor = Player;
        CurrentFlySpeed = FlySpeed; // 初始化速度

        // 动态激活 Tick，开始飞向玩家
        SetActorTickEnabled(true);

        // 为了防止多次触发重叠，关闭原有的碰撞体通道（但保留与主角的 overlap 用于最后吃掉）
        if (CollisionSphere)
        {
            CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
            CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        }
    }
}