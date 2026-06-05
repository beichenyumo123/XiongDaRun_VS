// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Coin.generated.h"

UCLASS()
class XIONGDARUN_V2_API ACoin : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ACoin();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // 碰撞球体（作为根组件，用于检测玩家）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* CollisionSphere;

    // 金币的视觉网格体
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* CoinMesh;

    // 碰撞事件处理函数
    UFUNCTION()
    void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    // --- 架构精髓：C++ 通知蓝图播放特效 ---
    UFUNCTION(BlueprintImplementableEvent, Category = "Coin|Events")
    void OnCoinCollectedBP();

    // ==========================================
    // --- 新增：磁铁吸附接口 ---
    // ==========================================

    // 令金币向指定的 Actor（玩家）飞去
    UFUNCTION(BlueprintCallable, Category = "Coin|Magnet")
    void AttractTo(AActor* Player);

protected:
    // 被吸引的目标
    UPROPERTY(BlueprintReadOnly, Category = "Coin|Magnet")
    AActor* TargetActor;

    // 飞行的基础速度（厘米/秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Magnet")
    float FlySpeed = 1200.0f;

    // 飞行加速度因子，让飞行产生越来越快的“吸附感”
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Magnet")
    float FlyAcceleration = 1500.0f;

    // 运行时的实际速度
    float CurrentFlySpeed = 0.0f;

    // --- 新增：金币类型的得分配置 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Config")
    int32 CoinValue = 1;
};