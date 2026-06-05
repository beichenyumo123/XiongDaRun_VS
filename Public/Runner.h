// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Runner.generated.h"

// 前向声明摄像机与碰撞组件
class USpringArmComponent;
class UCameraComponent;
class USphereComponent;
class UCameraShakeBase; // 前向声明相机抖动类

UCLASS()
class XIONGDARUN_V2_API ARunner : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARunner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// --- 蓝图可调用的控制接口 ---

	UFUNCTION(BlueprintCallable, Category = "Runner|Movement")
	void MoveLeft();

	UFUNCTION(BlueprintCallable, Category = "Runner|Movement")
	void MoveRight();

	// --- 修改：支持传入金币数量/分值并支持连击与狂暴乘数 ---
	UFUNCTION(BlueprintCallable, Category = "Runner|Score")
	void AddCoin(int32 Amount = 1);

	// 获取当前金币数 (BlueprintPure 纯只读获取节点)
	UFUNCTION(BlueprintPure, Category = "Runner|Score")
	int32 GetCoinCount() const { return CoinCount; }

	// --- 核心新增：距离与得分系统 ---

	// 获取当前跑过的距离（米）
	UFUNCTION(BlueprintPure, Category = "Runner|Score")
	int32 GetDistanceRun() const { return DistanceMeters; }

	// 获取综合总分（距离分 + 连击加成金币分）
	UFUNCTION(BlueprintPure, Category = "Runner|Score")
	int32 GetTotalScore() const;

	// 标记主角是否死亡
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	// 死亡处理函数，暴露给蓝图以便后续可以触发死亡UI
	UFUNCTION(BlueprintCallable, Category = "State")
	void Die();

	// --- 新增：通知蓝图玩家已死亡，用于弹出 UI ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events")
	void OnPlayerDiedBP();

	// 激活磁铁状态（供磁铁道具调用）
	UFUNCTION(BlueprintCallable, Category = "Runner|Powerups")
	void ActivateMagnet();

	// 熄灭磁铁状态
	void DeactivateMagnet();

	// 获取当前磁吸状态是否开启
	UFUNCTION(BlueprintPure, Category = "Runner|Powerups")
	bool bIsMagnetActiveState() const { return bIsMagnetActive; }


protected:

	// --- 新增：摄像机与弹簧臂 ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* MagnetSphere;

	// --- 核心状态与参数配置 ---

	// 当前轨道：0=左, 1=中, 2=右。默认生成在中间。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|State")
	int32 CurrentLane;

	// 轨道之间的宽度距离 (你可以根据你跑道模型的实际宽度在蓝图中调整)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config")
	float LaneWidth;

	// 变道的平滑过渡速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config")
	float SwitchLaneInterpSpeed;

	// 新增：向前的基础奔跑速度倍率
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config")
	float ForwardRunSpeed;

	// 新增：记录吃到的金币总数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Score")
	int32 CoinCount = 0;


	// 磁铁有效持续时间（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config|Magnet")
	float MagnetDuration = 8.0f;

	// 磁吸范围半径
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config|Magnet")
	float MagnetRadius = 800.0f;

	// 标记当前磁吸是否开启
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|State|Magnet")
	bool bIsMagnetActive = false;

	// =========================================================================
	// --- 验收冲刺：动态难度、得分参数与相机果汁效果 ---
	// =========================================================================

	// 起始/最低奔跑速度（厘米/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed")
	float MinSpeedLimit = 700.0f;

	// 最大奔跑速度上限（厘米/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed")
	float MaxSpeedLimit = 1800.0f;

	// 速度随时间递增的加速度（每秒增加多少厘米/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed")
	float SpeedAccelerationRate = 18.0f;

	// 记录当前的实时最大移动速度
	float CurrentMaxWalkSpeed;

	// 初始 Y 轴插值速度，用于高速时按比例提升变道灵敏度
	float InitialSwitchLaneInterpSpeed;

	// 记录跑步的起点 X 坐标
	float StartX;

	// 当前已跑跑酷距离（米）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Score")
	int32 DistanceMeters = 0;

	// 一米折算多少分
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Score")
	int32 ScorePerMeter = 10;

	// 一个金币折算的基本分
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Score")
	int32 ScorePerCoinUnit = 100;

	// 累加的金币与连击得分
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Score")
	int32 AccumulativeCoinScore = 0;

	// --- 视觉张力提升：视角拉伸（FOV）与相机抖动 ---

	// 正常速度下的基础相机 FOV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice")
	float BaseFOV = 90.0f;

	// 极限速度下的最大相机 FOV（产生超速拉伸感）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice")
	float MaxSpeedFOV = 110.0f;

	// 吃金币时的轻微相机抖动模板类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice")
	TSubclassOf<UCameraShakeBase> CoinCollectShakeClass;

	// 撞击死亡时的剧烈相机抖动模板类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice")
	TSubclassOf<UCameraShakeBase> DeathShakeClass;

	// 配置需要无限跟随时差的大山 Actor 数组
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Background")
	TArray<AActor*> BackgroundActors;

	// =========================================================================
	// --- 验收爆点：变道相机侧倾、连击Combo、狂暴FeverMode 变量配置 ---
	// =========================================================================

	// 1. 变道相机侧倾参数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Lean")
	float CameraLeanSensitivity = 0.025f; // 侧倾灵敏度，值越大倾斜越明显

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Lean")
	float MaxCameraLeanRoll = 6.0f; // 相机最大侧倾 Roll 角度限制

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Lean")
	float CameraLeanInterpSpeed = 8.0f; // 侧倾平滑插值过渡速度

	// 2. 连击 Combo 参数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Juice|Combo")
	int32 CurrentComboCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Juice|Combo")
	float ComboValidWindow = 2.0f; // 连击有效判定窗口时间（秒）

	FTimerHandle ComboResetTimerHandle; // 连击失效计时器

	// 3. 狂暴 Fever Mode 参数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Juice|Fever")
	bool bIsFeverModeActive = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Juice|Fever")
	float FeverModeDuration = 5.00f; // 狂暴状态持续时间（秒）

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Juice|Fever")
	int32 FeverComboThreshold = 10; // 触发狂暴状态的 Combo 门槛（10连击）

	FTimerHandle FeverDurationTimerHandle; // 狂暴倒计时器

	// --- 蓝图通知事件：让美术/蓝图根据这些亮点播放极其炫酷的视觉特效 ---

	// 连击数更新事件（用于更新 UI 界面上的“Combo x5!”以及数字放大抖动）
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events|Juice")
	void OnComboUpdatedBP(int32 NewCombo);

	// 连击失效事件（UI 连击淡出）
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events|Juice")
	void OnComboResetBP();

	// 狂暴模式激活事件（用于在角色脚底生成炫酷火花/屏幕边缘泛红/狂暴背景音乐切换）
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events|Juice")
	void OnFeverModeActivatedBP();

	// 狂暴结束事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events|Juice")
	void OnFeverModeDeactivatedBP();

private:
	// 目标 Y 轴坐标（左右平移改变的是 Y 轴）
	float TargetY;

	// 控制磁铁结束的计时器句柄
	FTimerHandle MagnetTimerHandle;

	// 磁吸检测球的碰撞回调（检测到金币并将其吸过来）
	UFUNCTION()
	void OnMagnetSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 内部播放相机抖动的辅助工具
	void PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.0f);

	// 连击过期回调
	void ResetCombo();

	// 狂暴结束回调
	void DeactivateFeverMode();
};