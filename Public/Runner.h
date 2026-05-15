// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Runner.generated.h"

// 前向声明摄像机组件
class USpringArmComponent;
class UCameraComponent;

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

	// --- 新增：金币收集逻辑 ---
	UFUNCTION(BlueprintCallable, Category = "Runner|Score")
	void AddCoin();

	// 获取当前金币数 (BlueprintPure 表示这是一个没有任何副作用的只读获取节点，纯绿色节点)
	UFUNCTION(BlueprintPure, Category = "Runner|Score")
	int32 GetCoinCount() const { return CoinCount; }

	// 标记主角是否死亡
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	// 死亡处理函数，暴露给蓝图以便后续可以触发死亡UI
	UFUNCTION(BlueprintCallable, Category = "State")
	void Die();

protected:

	// --- 新增：摄像机与弹簧臂 ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

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

	// 新增：向前的基础奔跑速度倍率 (用于微调，实际速度主要受 CharacterMovement 的 MaxWalkSpeed 影响)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config")
	float ForwardRunSpeed;
	// 新增：记录吃到的金币总数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Score")
	int32 CoinCount = 0;
private:
	// 目标 Y 轴坐标（我们假设跑酷是沿着 X 轴向前跑，所以左右平移改变的是 Y 轴）
	float TargetY;
};
