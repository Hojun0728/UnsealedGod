// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/Actor.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "BossWarningDecal.generated.h"

// ============================================================================
// ABossWarningDecal
// 보스 범위 공격의 "바닥 예고" 데칼. (수업 코드 ADecalBase 복습)
// 바닥에 원을 그리고, 공격 순간까지 남은 시간에 맞춰 머티리얼의 Progress(0 → 1)를 올린다.
//   → 원이 다 차오르는 순간 = 공격이 떨어지는 순간. 플레이어가 "언제 피해야 하는지" 보고 배울 수 있다.
//
// 사용 : SpawnActor로 바닥 위치에 만들고 StartWarning(머티리얼, 반지름, 시간)을 부른다.
//        공격이 끝나면 만든 쪽(패턴 어빌리티)이 Destroy한다.
//
// 머티리얼 조건 : 머티리얼 도메인 = 디퍼드 데칼, 스칼라 파라미터 "Progress"
//   (에디터에서 Content/UnsealedGod/Boss/M_Decal_Warning으로 만든다)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API ABossWarningDecal : public AActor
{
	GENERATED_BODY()

public:
	ABossWarningDecal();

protected:
	// 루트. 데칼은 이 아래에 붙여서 "아래를 보도록" 회전시킨다.
	// (수업 코드처럼 데칼을 루트로 쓰면, SpawnActor에 넘긴 회전값이 데칼의 아래 방향 회전을 덮어쓴다)
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent>	mRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent>	mDecal;

	// 실행 중에 파라미터(Progress)를 바꾸려면 머티리얼의 "다이내믹 인스턴스"가 필요하다. (수업 코드와 동일)
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic>	mDynamicMaterial;

	// 예고 시간(초)과 지금까지 흐른 시간
	float	mDuration = 1.f;
	float	mElapsed = 0.f;

public:
	virtual void Tick(float DeltaTime) override;

	// 원 예고 시작. Material : 데칼 머티리얼, Radius : 원 반지름(cm, 판정 반지름과 같게), Duration : 공격까지 남은 시간(초)
	void StartWarning(UMaterialInterface* Material, float Radius, float Duration);

	// 사각형 예고 시작. 액터의 앞(X) 방향이 사각형의 "길이" 방향이다. → 스폰할 때 회전값(Yaw)으로 방향을 정한다.
	// HalfLength : 앞뒤 반 길이, HalfWidth : 좌우 반 폭 (cm)
	void StartWarningBox(UMaterialInterface* Material, float HalfLength, float HalfWidth, float Duration);

protected:
	// 두 시작 함수의 공통 부분 : 머티리얼 적용 + Progress 초기화
	void ApplyMaterial(UMaterialInterface* Material, float Duration);
};
