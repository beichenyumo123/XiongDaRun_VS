// Fill out your copyright notice in the Description page of Project Settings.


#include "Coin.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runner.h" // 引入主角的头文件

// Sets default values
ACoin::ACoin()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

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

}

void ACoin::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 如果碰到的是玩家角色
    if (OtherActor && OtherActor->IsA(ARunner::StaticClass()))
    {
        ARunner* Runner = Cast<ARunner>(OtherActor);
        if (Runner)
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