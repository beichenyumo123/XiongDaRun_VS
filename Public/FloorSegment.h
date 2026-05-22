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
class UHierarchicalInstancedStaticMeshComponent; // 引入 HISM 组件

// --- 新增：定义轨道上可生成的物品类型 ---
UENUM(BlueprintType)
enum class ESpawnItemType : uint8
{
	None       UMETA(DisplayName = "空 (Empty)"),
	Coin       UMETA(DisplayName = "金币 (Coin)"),
	Obstacle   UMETA(DisplayName = "障碍物 (Obstacle)")
};
// --- 新增：定义一排的生成阵型 ---
USTRUCT(BlueprintType)
struct FRowSpawnPattern
{
	GENERATED_BODY()

	// 规定长度必须是3，分别对应 左、中、右 轨道
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
	TArray<ESpawnItemType> LaneItems;

	FRowSpawnPattern()
	{
		// 默认初始化为3个空位
		LaneItems.Init(ESpawnItemType::None, 3);
	}
};

// --- 核心升级：定义一个区块（连续多排）的预制件 ---
USTRUCT(BlueprintType)
struct FChunkSpawnPattern
{
	GENERATED_BODY()

	// 包含连续多排的阵型（比如可以配置成一个 S型金币轨迹 或 跨栏组合）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
	TArray<FRowSpawnPattern> Rows;
};

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

	// --- 修改：支持多种森林环境装饰 (动态 HISM) ---
	// 在蓝图中配置多种模型（不同种类的树、石头、草丛）
	UPROPERTY(EditAnywhere, Category = "Environment")
	TArray<class UStaticMesh*> EnvironmentMeshes;

	// 运行时动态生成的 HISM 组件列表（不需要暴露给蓝图）
	UPROPERTY()
	TArray<UHierarchicalInstancedStaticMeshComponent*> EnvironmentHISMs;

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

	// --- 修改：允许蓝图配置的安全区块阵型库 ---
	UPROPERTY(EditAnywhere, Category = "Spawner|Patterns")
	TArray<FChunkSpawnPattern> ChunkPatterns;

	// --- 新增：允许蓝图配置的安全阵型库 ---
	UPROPERTY(EditAnywhere, Category = "Spawner|Patterns")
	TArray<FRowSpawnPattern> SafePatterns;

	// --- 森林生成配置 ---
	UPROPERTY(EditAnywhere, Category = "Environment")
	int32 TreesPerSide = 4; // 每侧生成几棵树

	UPROPERTY(EditAnywhere, Category = "Environment")
	float TreeSpawnOffset = 600.0f; // 树木距离跑道中心的距离 (Y轴偏移)

	// 生成两侧树木的函数
	void SpawnEnvironment();

public: // <-- 注意这里，我们要把 SpawnItems 移到 public 下，让 GameMode 能调用它
	// 带有安全区参数的生成逻辑
	void SpawnItems(bool bIsSafeZone);
};
