// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FloorSegment.generated.h"

class UBoxComponent;
// 前向声明金币类
class UBoxComponent;
class ACoin;
class AObstacleBase;
UCLASS()
class XIONGDARUN_V2_API AFloorSegment : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFloorSegment();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 跑道的网格体组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* FloorMesh;

	// 附着点组件，用于标记下一个跑道生成的起点位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* AttachPoint;

	// 新增：触发器盒子，玩家踩到这里就会触发生成和销毁
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	// 获取当前跑道末端的位置（世界坐标和旋转）
	UFUNCTION(BlueprintCallable, Category = "Floor")
	FTransform GetAttachTransform() const;

protected:
	// 新增：处理重叠事件的函数。
	// 注意：绑定到 UE 碰撞系统的函数，必须加上 UFUNCTION() 宏！
	UFUNCTION()
	void OnTriggerBoxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// --- 物品生成配置 ---

	UPROPERTY(EditAnywhere, Category = "Spawner")
	class TSubclassOf<ACoin> CoinClass;

	// 新增：让蓝图配置生成哪种障碍物
	UPROPERTY(EditAnywhere, Category = "Spawner")
	TArray<TSubclassOf<AObstacleBase>> ObstacleClasses;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	float LaneWidth = 270.0f;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	int32 SpawnRows = 3;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	float FloorLength = 1000.0f;


public: // <-- 注意这里，我们要把 SpawnItems 移到 public 下，让 GameMode 能调用它
	// 带有安全区参数的生成逻辑
	void SpawnItems(bool bIsSafeZone);
};
