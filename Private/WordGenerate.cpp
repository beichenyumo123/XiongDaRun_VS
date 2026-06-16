// Fill out your copyright notice in the Description page of Project Settings.

#include "WordGenerate.h"
#include "FloorSegment.h" // �����ܵ�Ƭ�����ͷ�ļ�
#include "Engine/World.h"

AWordGenerate::AWordGenerate()
{
    // ��ʼ������λ�ã������ԭ�㿪ʼ��
    NextSpawnTransform = FTransform::Identity;
}

void AWordGenerate::BeginPlay()
{
    Super::BeginPlay();

    // ��Ϸ��ʼʱ�����ɳ�ʼ��һ���ܵ�
    if (FloorSegmentClass)
    {
        for (int32 i = 0; i < InitialFloorsToSpawn; ++i)
        {
            AddNewFloor();
        }
    }
}

void AWordGenerate::AddNewFloor()
{
    // ��ȫ��飺������粻���ڻ���δָ�����ɵ��࣬��ֱ�ӷ���
    UWorld* World = GetWorld();
    if (!World || !FloorSegmentClass) return;

    // �������ɲ����������ײ�赲��Ȼ���ɣ�
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // --- �����߼���Spawn Actor ---
    // ʹ�õ�ǰ��¼�� NextSpawnTransform ��λ�ú���ת������
    AFloorSegment* SpawnedFloor = World->SpawnActor<AFloorSegment>(
        FloorSegmentClass,
        NextSpawnTransform.GetLocation(),
        NextSpawnTransform.GetRotation().Rotator(),
        SpawnParams
    );

    if (SpawnedFloor)
    {
        // ������ɳɹ������� NextSpawnTransform��ʹ����ڸ����ɵ��ܵ���ĩ�˵�
        // ��һ�ε��� AddNewFloor ʱ���ͻ�������µ�λ������
        NextSpawnTransform = SpawnedFloor->GetAttachTransform();
        //�����޸ģ��ж��Ƿ�Ϊǰ 2 ���ܵ�����ȫ����
        bool bIsSafeZone = (SpawnedFloorCount < 2);

        // �� GameMode ���������ܵ�������Ʒ
        SpawnedFloor->SpawnItems(bIsSafeZone);

        // ��������һ
        SpawnedFloorCount++;

    }
}

