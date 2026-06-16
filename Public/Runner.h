// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Runner.generated.h"

// 前向声明
class USpringArmComponent;
class UCameraComponent;
class USphereComponent;
class UCameraShakeBase;
class UCurveFloat; // 难度曲线

// 难度曲线评估模式
UENUM(BlueprintType)
enum class ESpeedCurveMode : uint8
{
	Time       UMETA(DisplayName = "基于时间 (Time)"),
	Distance   UMETA(DisplayName = "基于距离 (Distance)")
};

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
	// --- ��ͼ�ɵ��õĿ��ƽӿ� ---

	UFUNCTION(BlueprintCallable, Category = "Runner|Movement")
	void MoveLeft();

	UFUNCTION(BlueprintCallable, Category = "Runner|Movement")
	void MoveRight();

	// --- �޸ģ�֧�ִ���������/��ֵ��֧��������񱩳��� ---
	UFUNCTION(BlueprintCallable, Category = "Runner|Score")
	void AddCoin(int32 Amount = 1);

	// ��ȡ��ǰ����� (BlueprintPure ��ֻ����ȡ�ڵ�)
	UFUNCTION(BlueprintPure, Category = "Runner|Score")
	int32 GetCoinCount() const { return CoinCount; }

	// --- ����������������÷�ϵͳ ---

	// ��ȡ��ǰ�ܹ��ľ��루�ף�
	UFUNCTION(BlueprintPure, Category = "Runner|Score")
	int32 GetDistanceRun() const { return DistanceMeters; }

	// ��ȡ�ۺ��ܷ֣������ + �����ӳɽ�ҷ֣�
	UFUNCTION(BlueprintPure, Category = "Runner|Score")
	int32 GetTotalScore() const;

	// ��������Ƿ�����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	// ����������������¶����ͼ�Ա�������Դ�������UI
	UFUNCTION(BlueprintCallable, Category = "State")
	void Die();

	// --- ������֪ͨ��ͼ��������������ڵ��� UI ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events")
	void OnPlayerDiedBP();

	// �������״̬�����������ߵ��ã�
	UFUNCTION(BlueprintCallable, Category = "Runner|Powerups")
	void ActivateMagnet();

	// Ϩ�����״̬
	void DeactivateMagnet();

	// ��ȡ��ǰ����״̬�Ƿ���
	UFUNCTION(BlueprintPure, Category = "Runner|Powerups")
	bool bIsMagnetActiveState() const { return bIsMagnetActive; }


protected:

	// --- ������������뵯�ɱ� ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* MagnetSphere;

	// --- ����״̬��������� ---

	// ��ǰ�����0=��, 1=��, 2=�ҡ�Ĭ���������м䡣
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|State")
	int32 CurrentLane;

	// ���֮��Ŀ��Ⱦ��� (����Ը������ܵ�ģ�͵�ʵ�ʿ�������ͼ�е���)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config")
	float LaneWidth;

	// �����ƽ�������ٶ�
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config")
	float SwitchLaneInterpSpeed;

	// ��������ǰ�Ļ��������ٶȱ���
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config")
	float ForwardRunSpeed;

	// ��������¼�Ե��Ľ������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Score")
	int32 CoinCount = 0;


	// ������Ч����ʱ�䣨�룩
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config|Magnet")
	float MagnetDuration = 8.0f;

	// ������Χ�뾶
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Config|Magnet")
	float MagnetRadius = 800.0f;

	// ��ǵ�ǰ�����Ƿ���
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|State|Magnet")
	bool bIsMagnetActive = false;

	// =========================================================================
	// --- 难度曲线配置（支持非线性速度增长） ---
	// =========================================================================

	// 初始/最低奔跑速度（厘米/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed")
	float MinSpeedLimit = 700.0f;

	// 最高奔跑速度上限（厘米/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed")
	float MaxSpeedLimit = 1800.0f;

	// 速度随时间增长的加速度（每秒增加多少厘米/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed")
	float SpeedAccelerationRate = 18.0f;

	// 记录当前的实际移动速度
	float CurrentMaxWalkSpeed;

	// --- 难度曲线配置 ---

	// 曲线评估模式
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed|Curve")
	ESpeedCurveMode SpeedCurveMode = ESpeedCurveMode::Time;

	// 是否使用难度曲线（如果为false，使用线性加速度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed|Curve")
	bool bUseDifficultyCurve = false;

	// 难度曲线资产（X轴为时间/距离，Y轴为速度倍率0-1）
	// 曲线值0表示MinSpeedLimit，1表示MaxSpeedLimit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed|Curve", meta = (EditCondition = "bUseDifficultyCurve"))
	UCurveFloat* DifficultyCurve = nullptr;

	// 曲线评估的时间/距离缩放（控制曲线播放速度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed|Curve", meta = (EditCondition = "bUseDifficultyCurve"))
	float CurveTimeScale = 1.0f;

	// 曲线评估的时间/距离偏移（控制曲线开始时间）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed|Curve", meta = (EditCondition = "bUseDifficultyCurve"))
	float CurveTimeOffset = 0.0f;

	// 曲线插值平滑速度（值越大，跟随曲线越快）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Speed|Curve", meta = (EditCondition = "bUseDifficultyCurve"))
	float CurveInterpSpeed = 5.0f;

	// ��ʼ Y ���ֵ�ٶȣ����ڸ���ʱ�������������������
	float InitialSwitchLaneInterpSpeed;

	// ��¼�ܲ������ X ����
	float StartX;

	// ��ǰ�����ܿ���루�ף�
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Score")
	int32 DistanceMeters = 0;

	// һ��������ٷ�
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Score")
	int32 ScorePerMeter = 10;

	// һ���������Ļ�����
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Score")
	int32 ScorePerCoinUnit = 100;

	// �ۼӵĽ���������÷�
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Score")
	int32 AccumulativeCoinScore = 0;

	// --- �Ӿ������������ӽ����죨FOV����������� ---

	// �����ٶ��µĻ������ FOV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice")
	float BaseFOV = 90.0f;

	// �����ٶ��µ������� FOV��������������У�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice")
	float MaxSpeedFOV = 110.0f;

	// �Խ��ʱ����΢�������ģ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice")
	TSubclassOf<UCameraShakeBase> CoinCollectShakeClass;

	// ײ������ʱ�ľ����������ģ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice")
	TSubclassOf<UCameraShakeBase> DeathShakeClass;

	// ������Ҫ���޸���ʱ��Ĵ�ɽ Actor ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Background")
	TArray<AActor*> BackgroundActors;

	// 缓存背景Actor的根组件指针，避免每帧重复获取（性能优化）
	UPROPERTY()
	TArray<USceneComponent*> CachedBackgroundRootComponents;

	// =========================================================================
	// --- ���ձ��㣺���������㡢����Combo����FeverMode �������� ---
	// =========================================================================

	// 1. �������������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Lean")
	float CameraLeanSensitivity = 0.025f; // ���������ȣ�ֵԽ����бԽ����

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Lean")
	float MaxCameraLeanRoll = 6.0f; // ��������� Roll �Ƕ�����

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Lean")
	float CameraLeanInterpSpeed = 8.0f; // ����ƽ����ֵ�����ٶ�

	// 2. ���� Combo ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Juice|Combo")
	int32 CurrentComboCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Juice|Combo")
	float ComboValidWindow = 2.0f; // ������Ч�ж�����ʱ�䣨�룩

	FTimerHandle ComboResetTimerHandle; // ����ʧЧ��ʱ��

	// 3. �� Fever Mode ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runner|Juice|Fever")
	bool bIsFeverModeActive = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Juice|Fever")
	float FeverModeDuration = 5.00f; // ��״̬����ʱ�䣨�룩

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Runner|Juice|Fever")
	int32 FeverComboThreshold = 10; // ������״̬�� Combo �ż���10������

	FTimerHandle FeverDurationTimerHandle; // �񱩵���ʱ��

	// --- 狂暴模式视觉增强 ---

	// Fever期间FOV脉冲效果
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Fever|Visual")
	float FeverFOVBoost = 130.0f; // Fever激活时的FOV峰值

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Fever|Visual")
	float FeverFOVPulseSpeed = 6.0f; // FOV脉冲频率（越大越快）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Fever|Visual")
	float FeverFOVPulseAmount = 5.0f; // FOV脉冲幅度（±值）

	// Fever期间速度爆发
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Fever|Visual")
	float FeverSpeedBoostMultiplier = 1.3f; // Fever期间速度倍率（1.3 = 30%加速）

	// Fever期间持续震动强度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Fever|Visual")
	float FeverContinuousShakeScale = 0.15f; // 持续低频震动强度

	// Fever激活瞬间震动强度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runner|Juice|Fever|Visual")
	float FeverActivationShakeScale = 1.2f; // 激活瞬间的强震

	// --- ��ͼ֪ͨ�¼���������/��ͼ������Щ���㲥�ż����ſ���Ӿ���Ч ---

	// �����������¼������ڸ��� UI �����ϵġ�Combo x5!���Լ����ַŴ󶶶���
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events|Juice")
	void OnComboUpdatedBP(int32 NewCombo);

	// ����ʧЧ�¼���UI ����������
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events|Juice")
	void OnComboResetBP();

	// ��ģʽ�����¼��������ڽ�ɫ�ŵ������ſ��/��Ļ��Ե����/�񱩱��������л���
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events|Juice")
	void OnFeverModeActivatedBP();

	// �񱩽����¼�
	UFUNCTION(BlueprintImplementableEvent, Category = "Runner|Events|Juice")
	void OnFeverModeDeactivatedBP();

private:
	// Ŀ�� Y �����꣨����ƽ�Ƹı���� Y �ᣩ
	float TargetY;

	// ���ƴ��������ļ�ʱ�����
	FTimerHandle MagnetTimerHandle;

	// ������������ײ�ص�����⵽��Ҳ�������������
	UFUNCTION()
	void OnMagnetSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// �ڲ�������������ĸ�������
	void PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.0f);

	// �������ڻص�
	void ResetCombo();

	// �񱩽����ص�
	void DeactivateFeverMode();
};