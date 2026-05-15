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

    // 新增：跑道一生成，就立刻在自己身上撒金币
    //SpawnItems();
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


void AFloorSegment::SpawnItems(bool bIsSafeZone)
{
    if (!GetWorld()) return;

    float Spacing = FloorLength / (SpawnRows + 1);

    for (int32 Row = 1; Row <= SpawnRows; Row++)
    {
        int32 ObstacleCount = 0;

        for (int32 Lane = 0; Lane < 3; Lane++)
        {
            bool bSpawnObstacle = false;
            bool bSpawnCoin = false;

            //核心修改：检查 ObstacleClasses 数组里是否有东西
            if (!bIsSafeZone && ObstacleClasses.Num() > 0 && ObstacleCount < 2 && FMath::FRand() < 0.2f)
            {
                bSpawnObstacle = true;
                ObstacleCount++;
            }
            // 金币不受安全区影响，一开始就可以吃金币
            else if (CoinClass && FMath::FRand() < 0.4f)
            {
                bSpawnCoin = true;
            }

            // --- 开始生成物体 ---
            if (bSpawnObstacle || bSpawnCoin)
            {
                float YOffset = (Lane - 1) * LaneWidth;

                // Z=50 是为了防止穿模，如果障碍物模型比较高，你可以在蓝图里调，或者这里再加高点
                FVector LocalLocation(Spacing * Row, YOffset, 40.0f);
                FVector WorldLocation = GetActorTransform().TransformPosition(LocalLocation);

                if (bSpawnObstacle)
                {
                    int32 RandomIndex = FMath::RandRange(0, ObstacleClasses.Num() - 1);
                    TSubclassOf<AObstacleBase> SelectedObstacleClass = ObstacleClasses[RandomIndex];

                    // 如果抽到的类不为空，才进行生成
                    if (SelectedObstacleClass)
                    {
                        GetWorld()->SpawnActor<AObstacleBase>(SelectedObstacleClass, WorldLocation, FRotator::ZeroRotator);
                    }
                }
                else if (bSpawnCoin)
                {
                    GetWorld()->SpawnActor<ACoin>(CoinClass, WorldLocation, FRotator::ZeroRotator);
                }
            }
        }
    }
}
