// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XiongDaRun_v2GameMode.h"
#include "WordGenerate.generated.h"

// 前向声明我们刚才创建的类
class AFloorSegment;

UCLASS()
class XIONGDARUN_V2_API AWordGenerate : public AXiongDaRun_v2GameMode
{
    GENERATED_BODY()

public:
    AWordGenerate();

protected:
    // GameMode 开始时调用
    virtual void BeginPlay() override;

public:
    // 暴露给蓝图，可以在蓝图里直接调用生成跑道
    UFUNCTION(BlueprintCallable, Category = "LevelGeneration")
    void AddNewFloor();

protected:
    // --- 配置属性 ---

    // 在蓝图中配置要生成的跑道片段类 (即你在步骤 2 创建的 BP_FloorSegment_Base)
    UPROPERTY(EditDefaultsOnly, Category = "LevelGeneration")
    TSubclassOf<AFloorSegment> FloorSegmentClass;

    // 初始生成多少个跑道片段
    UPROPERTY(EditDefaultsOnly, Category = "LevelGeneration")
    int32 InitialFloorsToSpawn = 5;

    // --- 状态变量 ---

    // 记录下一个跑道应该生成的 Transform (位置和旋转)
    // 初始值在构造函数或 BeginPlay 中设置
    FTransform NextSpawnTransform;
    //记录总共生成了多少段跑道
    int32 SpawnedFloorCount = 0;
};