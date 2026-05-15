// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runner.h" 

// Sets default values
AObstacleBase::AObstacleBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
    // 1. 设置根组件
    USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootComp;

    // 2. 设置模型
    ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObstacleMesh"));
    ObstacleMesh->SetupAttachment(RootComponent);

    // 3. 设置碰撞盒
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetupAttachment(RootComponent);
    // 障碍物的碰撞逻辑通常只需要 Query (检测重叠)，不需要 Physics (物理弹开)
    CollisionBox->SetCollisionProfileName(TEXT("Trigger"));
}

// Called when the game starts or when spawned
void AObstacleBase::BeginPlay()
{
	Super::BeginPlay();
    // 绑定重叠事件
    if (CollisionBox)
    {
        CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AObstacleBase::OnObstacleOverlap);
    }
}

// Called every frame
void AObstacleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AObstacleBase::OnObstacleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 安全检查：防止自身或空指针触发
    if (OtherActor && OtherActor != this)
    {
        // 尝试将撞上来的物体 Cast (类型转换) 为我们的主角
        ARunner* Runner = Cast<ARunner>(OtherActor);

        if (Runner)
        {
            // 如果成功，说明真的是主角撞上来了，执行主角的 Die 函数！
            Runner->Die();

            // 可以选择播放一声爆炸音效或特效（后续在蓝图里加）
        }
    }
}

