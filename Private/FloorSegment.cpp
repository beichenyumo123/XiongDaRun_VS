// Fill out your copyright notice in the Description page of Project Settings.


#include "FloorSegment.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h" // 引入 BoxComponent
#include "GameFramework/Character.h" // 引入 Character 用于判断玩家
#include "Kismet/GameplayStatics.h"  // 引入 GameplayStatics 用于获取 GameMode
#include "WordGenerate.h"            // 引入我们的 GameMode
#include "Coin.h" 
#include "ObstacleBase.h"
#include "MagnetItem.h"              // 引入新建的磁铁头文件
#include "Components/HierarchicalInstancedStaticMeshComponent.h" // 引入 HISM

// Sets default values
AFloorSegment::AFloorSegment()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    // 初始化根组件
    USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootComp;

    // 初始化跑道网格体
    FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
    FloorMesh->SetupAttachment(RootComponent);

    // 初始化附着点 (用于连接下一个跑道)
    AttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AttachPoint"));
    AttachPoint->SetupAttachment(RootComponent);
    // 注意：默认情况下 AttachPoint 只是一个点，你需要在蓝图中将其移动到跑道模型的末端。

    // --- 新增：初始化触发器 ---
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(RootComponent);
    // 设置碰撞预设为 Trigger (只检测重叠，不产生物理阻挡)
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

}

// Called when the game starts or when spawned
void AFloorSegment::BeginPlay()
{
	Super::BeginPlay();
    // 在游戏开始时，将我们的 C++ 函数绑定到触发器的“开始重叠”事件上
    if (TriggerBox)
    {
        TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AFloorSegment::OnTriggerBoxOverlap);
    }
    // --- 核心修改：动态创建 HISM 组件池 ---
// 根据蓝图中配置的模型数组，动态创建对应的 HISM 组件
    for (UStaticMesh* Mesh : EnvironmentMeshes)
    {
        if (Mesh)
        {
            // 在运行时动态生成组件
            UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            NewHISM->SetStaticMesh(Mesh);
            NewHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 背景不需要物理碰撞
            NewHISM->SetupAttachment(RootComponent);
            NewHISM->RegisterComponent(); // 动态创建的组件必须注册才能在场景中渲染

            // 存入数组供后续生成调用
            EnvironmentHISMs.Add(NewHISM);
        }
    }
    // 跑道生成时，同时在两侧生成森林背景
    SpawnEnvironment();
}

// Called every frame
void AFloorSegment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// 核心业务逻辑：碰撞触发后执行
void AFloorSegment::OnTriggerBoxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 安全检查：确保碰到它的 Actor 是存在的，且不是自己，且必须是玩家的角色 (ACharacter)
    // 这样可以防止发射的子弹或掉落的石头意外触发跑道生成
    if (OtherActor && OtherActor != this && OtherActor->IsA(ACharacter::StaticClass()))
    {
        // 1. 获取我们的 GameMode，并强制转换 (Cast) 为 AWordGenerate
        AWordGenerate* GameMode = Cast<AWordGenerate>(UGameplayStatics::GetGameMode(GetWorld()));
        if (GameMode)
        {
            // 调用 GameMode 的方法在前方生成一段新跑道
            GameMode->AddNewFloor();
        }

        // 2. 为了防止玩家后退再次触发，一旦触发过一次，就销毁这个触发器的碰撞
        TriggerBox->DestroyComponent();

        // 3. 延时自毁：UE 引擎自带的生命周期管理 API
        // 设置这个跑道 Actor 的寿命为 2.0 秒，2秒后引擎会自动调用 Destroy() 回收它释放内存
        SetLifeSpan(2.0f);
    }
}

FTransform AFloorSegment::GetAttachTransform() const
{
    if (AttachPoint)
    {
        // 返回附着点在世界空间中的变换（包含位置、旋转、缩放）
        return AttachPoint->GetComponentTransform();
    }

    // 如果没有附着点，返回 Actor 自身的变换作为后备
    return GetActorTransform();
}

// --- 核心修改：使用区块阵型驱动生成 ---
void AFloorSegment::SpawnItems(bool bIsSafeZone)
{
    if (!GetWorld()) return;

    // 1. 如果是安全区（前几段跑道），只在中间生成少量金币作为引导
    if (bIsSafeZone)
    {
        float SafeSpacing = FloorLength / (SpawnRows + 1);
        for (int32 Row = 1; Row <= SpawnRows; Row++)
        {
            if (CoinClass && FMath::FRand() < 0.5f)
            {
                FVector LocalLocation(SafeSpacing * Row, 0.0f, 40.0f);
                FVector WorldLocation = GetActorTransform().TransformPosition(LocalLocation);
                GetWorld()->SpawnActor<ACoin>(CoinClass, WorldLocation, FRotator::ZeroRotator);
            }
        }
        return;
    }

    // 2. 非安全区：抽取一个完整的区块预制件（Chunk）
    if (ChunkPatterns.Num() == 0) return;

    // 随机抽取一个区块剧本（例如：波浪金币组、左躲右闪障碍组）
    int32 RandomChunkIndex = FMath::RandRange(0, ChunkPatterns.Num() - 1);
    FChunkSpawnPattern SelectedChunk = ChunkPatterns[RandomChunkIndex];

    // 实际生成的排数取决于你在蓝图里为这个 Chunk 配置了多少排
    int32 ActualRows = SelectedChunk.Rows.Num();
    if (ActualRows == 0) return;

    // 动态计算行间距，确保整个区块均匀分布在这段跑道上
    float Spacing = FloorLength / (ActualRows + 1);

    for (int32 RowIndex = 0; RowIndex < ActualRows; RowIndex++)
    {
        FRowSpawnPattern CurrentPattern = SelectedChunk.Rows[RowIndex];
        int32 Row = RowIndex + 1;

        // 解析当前排的左中右轨道
        for (int32 Lane = 0; Lane < 3; Lane++)
        {
            if (Lane < CurrentPattern.LaneItems.Num())
            {
                ESpawnItemType ItemToSpawn = CurrentPattern.LaneItems[Lane];

                float YOffset = (Lane - 1) * LaneWidth;
                FVector LocalLocation(Spacing * Row, YOffset, 40.0f);
                FVector WorldLocation = GetActorTransform().TransformPosition(LocalLocation);

                if (ItemToSpawn == ESpawnItemType::Obstacle && ObstacleClasses.Num() > 0)
                {
                    int32 RandomIndex = FMath::RandRange(0, ObstacleClasses.Num() - 1);
                    TSubclassOf<AObstacleBase> SelectedObstacleClass = ObstacleClasses[RandomIndex];

                    if (SelectedObstacleClass)
                    {
                        GetWorld()->SpawnActor<AObstacleBase>(SelectedObstacleClass, WorldLocation, FRotator::ZeroRotator);
                    }
                }
                else if (ItemToSpawn == ESpawnItemType::Coin && CoinClass)
                {
                    GetWorld()->SpawnActor<ACoin>(CoinClass, WorldLocation, FRotator::ZeroRotator);
                }
                // --- 新增：处理生成吸铁石道具逻辑 ---
                else if (ItemToSpawn == ESpawnItemType::Magnet && MagnetClass)
                {
                    GetWorld()->SpawnActor<AMagnetItem>(MagnetClass, WorldLocation, FRotator::ZeroRotator);
                }
            }
        }
    }
}

// --- 核心新增：生成森林环境 ---
void AFloorSegment::SpawnEnvironment()
{
    // 如果没有配置任何环境模型（或者 HISM 组件没生成成功），直接跳过
    if (EnvironmentHISMs.Num() == 0) return;

    // 计算沿跑道方向每棵树的间距
    float Spacing = FloorLength / TreesPerSide;

    for (int32 i = 0; i < TreesPerSide; i++)
    {
        // X 轴的基础位置
        float BaseX = i * Spacing + (Spacing * 0.5f);

        // 为了自然，给位置加一点随机性，不要排列得像电线杆一样死板
        float RandomXOffset = FMath::RandRange(-50.0f, 50.0f);
        float RandomYOffset = FMath::RandRange(-100.0f, 100.0f);

        // 随机缩放和旋转
        float RandomScale = FMath::RandRange(0.8f, 1.5f);
        FRotator RandomRot(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);

        // --- 1. 生成左侧的装饰物 ---
        // 随机选一种模型对应的 HISM 组件
        int32 RandomLeftIndex = FMath::RandRange(0, EnvironmentHISMs.Num() - 1);

        FVector LeftLocation(BaseX + RandomXOffset, -TreeSpawnOffset + RandomYOffset, 0.0f);
        FTransform LeftTransform(RandomRot, LeftLocation, FVector(RandomScale));
        EnvironmentHISMs[RandomLeftIndex]->AddInstance(LeftTransform);

        // --- 2. 生成右侧的装饰物 ---
        // 重新随机一下大小、旋转和模型种类，让左右不一样
        RandomScale = FMath::RandRange(0.8f, 1.5f);
        RandomRot = FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
        int32 RandomRightIndex = FMath::RandRange(0, EnvironmentHISMs.Num() - 1);

        FVector RightLocation(BaseX + RandomXOffset, TreeSpawnOffset + RandomYOffset, 0.0f);
        FTransform RightTransform(RandomRot, RightLocation, FVector(RandomScale));
        EnvironmentHISMs[RandomRightIndex]->AddInstance(RightTransform);
    }
}