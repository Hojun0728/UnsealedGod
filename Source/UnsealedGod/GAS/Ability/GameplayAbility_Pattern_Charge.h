// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_Charge.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_Charge
// 세바로그의 돌진 (모든 페이즈, 3페이즈부터 2배속). 중거리(4~12m)에서 직선으로 질주한 뒤 망치로 내려찍는다.
// 도약(포물선 + 원 범위)과 달리 "직선 + 빠름"이라, 옆으로 피하는 법을 배우는 패턴이다.
//
// 몽타주 AM_Sevarog_Charge (섹션 3개)
//   Default(준비) : Swing1Start (망치를 든 채 멈춤) — 바닥에 사각형 예고가 차오름
//   Run(달리기)   : Sprint_Fwd, 자기 자신으로 무한 반복. 몸통 판정(Melee Hit Window, spine_02, ×1.0)
//   Swing(마무리) : Swing1_Slow → Return2Idle. 망치 판정(×2.5)
//
// 흐름
//   활성화 : 방향·목표 지점을 "지금" 고정, 사각형 예고 데칼
//   → 돌진 시작 노티파이(Event.Boss.ChargeStart) : 몸끼리 부딪히지 않게 하고 직선 질주
//   → 도착(질주 시간 끝) : 몸 충돌 복구, Swing 섹션으로 점프
//
// 달리는 시간이 거리마다 달라서 몽타주 길이를 고정할 수 없다.
// → Run 섹션을 무한 반복시켜 두고, 코드가 도착 순간 Montage_JumpToSection으로 Swing에 넘긴다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_Charge : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_Charge();

protected:
	// ---------------------------------------------------------------- 수치
	// 달리기 속도(cm/초, 실제 시간). 기디언 이동 속도 600의 약 3.3배.
	// (10/15 : 15m/s는 느리다는 피드백으로 20m/s. 준비 시간은 그대로 두고 질주·마무리만 빠르게)
	float	mChargeSpeed = 2000.f;

	// 최대 질주 거리(cm)
	float	mMaxChargeDistance = 1200.f;

	// 준비 시간(초, "몽타주 시간" 기준) = 몽타주의 돌진 시작 노티파이 위치. 실제 시간 = 이 값 ÷ 재생 속도.
	// 예고 데칼이 이 시간 동안 차오르고, 다 차는 순간 출발한다.
	float	mWindupTime = 0.70f;

	// 예고 사각형의 좌우 반 폭(cm). 보스 몸통 판정(반지름 130)과 비슷하게.
	float	mPathHalfWidth = 125.f;

	// 예고 사각형을 멈추는 지점보다 얼마나 더 길게 그릴지(cm) = 마무리 망치가 닿는 곳까지.
	// (적정 타격 거리 250 + 망치 판정 반지름 80 ≈ 330 → 여유 있게 350)
	float	mPathExtraLength = 350.f;

	// 마무리 망치가 내려오는 시간(초, 몽타주 시간, Swing 섹션 시작 기준). 이때 예고 데칼을 지운다.
	float	mSwingHitTime = 0.5f;

	// 몽타주 섹션 이름 (몽타주에서 정한 이름과 대소문자까지 같아야 한다)
	FName	mSwingSectionName = TEXT("Swing");

	// ---------------------------------------------------------------- 실행 중 상태
	// 활성화 순간 고정한 질주 방향과 거리
	FVector	mChargeDirection = FVector::ForwardVector;
	float	mChargeDistance = 0.f;

	// 지금 몸 충돌을 끈 상태인지 (끝날 때 꼭 되돌리기 위해)
	bool	mPassThroughPawns = false;

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 몽타주의 Event.Boss.ChargeStart 노티파이 → 직선 질주 시작
	// virtual (10/28) : 오로라 얼음 돌진이 "질주 시작" 순간에 방패를 더하려고 덮어쓴다.
	UFUNCTION()
	virtual void OnChargeStartEvent(FGameplayEventData Payload);

	// 질주 끝(도착) → Swing 섹션으로
	UFUNCTION()
	void OnChargeFinished();

	// 마무리 망치가 내려온 시점 → 예고 데칼 제거
	UFUNCTION()
	void OnSwingHitTime();

	// 도착 → 마무리 섹션(세바로그 Swing 망치 / 오로라 Stop 멈춤)으로. virtual (10/28) : 얼음 돌진이 방패를 거두려고 덮어쓴다.
	virtual void StartSwing();

	// 보스가 기디언과 몸끼리 부딪히지 않게(true) / 원래대로(false)
	void SetPassThroughPawns(bool bPassThrough);

};
