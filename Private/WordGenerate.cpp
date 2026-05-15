// Fill out your copyright notice in the Description page of Project Settings.

#include "WordGenerate.h"
#include "FloorSegment.h" // 引入跑道片段类的头文件
#include "Engine/World.h"

AWordGenerate::AWordGenerate()
{
    // 初始化生成位置（假设从原点开始）
    NextSpawnTransform = FTransform::Identity;
}

void AWordGenerate::BeginPlay()
{
    Super::BeginPlay();

    // 游戏开始时，生成初始的一段跑道
    if (FloorSegmentClass)
    {
        for (int32 i = 0; i < InitialFloorsToSpawn; ++i)
        {
            AddNewFloor();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AWordGenerate: FloorSegmentClass is not assigned!"));
    }
}

void AWordGenerate::AddNewFloor()
{
    // 安全检查：如果世界不存在或者未指定生成的类，则直接返回
    UWorld* World = GetWorld();
    if (!World || !FloorSegmentClass) return;

    // 设置生成参数（如果碰撞阻挡依然生成）
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // --- 核心逻辑：Spawn Actor ---
    // 使用当前记录的 NextSpawnTransform 的位置和旋转来生成
    AFloorSegment* SpawnedFloor = World->SpawnActor<AFloorSegment>(
        FloorSegmentClass,
        NextSpawnTransform.GetLocation(),
        NextSpawnTransform.GetRotation().Rotator(),
        SpawnParams
    );

    if (SpawnedFloor)
    {
        // 如果生成成功，更新 NextSpawnTransform，使其等于刚生成的跑道的末端点
        // 下一次调用 AddNewFloor 时，就会用这个新的位置生成
        NextSpawnTransform = SpawnedFloor->GetAttachTransform();
        //核心修改：判断是否为前 2 段跑道（安全区）
        bool bIsSafeZone = (SpawnedFloorCount < 2);

        // 由 GameMode 主动命令跑道生成物品
        SpawnedFloor->SpawnItems(bIsSafeZone);

        // 计数器加一
        SpawnedFloorCount++;

        // （可选调试代码）打印生成信息
        UE_LOG(LogTemp, Log, TEXT("道路生成： %s"), *NextSpawnTransform.GetLocation().ToString());
    }
}

