// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_IceDecoy.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_IceDecoy
// 오로라의 얼음 분신 (원작 Frozen Simulacrum, 모든 페이즈). (10/28)
// 기디언이 가까이 붙으면 뒤로 공중제비로 빠지면서, 원래 자리에 "걸어오는 얼음 폭탄" 분신(AIceDecoy)을 남긴다.
//
// 몽타주 AM_Aurora_IceDecoy : Ability_RMB_Bwd (뒤로 공중제비). 4프레임에 Event.Boss.LeapTakeoff
// 흐름
//   이륙 노티파이 : 분신 생성(오로라 자리, 기디언 쪽으로 살짝 앞) → 기디언 반대쪽으로 mRetreatDistance 포물선 도약 (부모의 CreateJumpToTask)
//   → 착지(17프레임) → 몽타주 끝 → 후딜
// 1페이즈 분신 1개 / 2페이즈 2개(좌우로 갈라짐, 속도 380·420). 살아 있는 분신이 있으면 이 패턴을 쓰지 않는다
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_IceDecoy : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_IceDecoy();

protected:
	// ---------------------------------------------------------------- 수치 (10/28 직접 결정)
	float	mRetreatDistance = 600.f;		// 뒤로 빠지는 거리
	float	mJumpHeight = 150.f;
	float	mAirTime = 13.f / 30.f;			// 이륙(4프레임) ~ 착지(17프레임), 몽타주 시간
	float	mDecoyForwardOffset = 100.f;	// 분신을 오로라 자리보다 기디언 쪽으로 이만큼 앞에 (몸이 겹치지 않게)

	// 2페이즈 분신 2개 (10/28 내 판단) : 좌우 간격, 각각의 걷는 속도
	float	mTwinSideOffset = 150.f;
	TArray<float>	mTwinWalkSpeeds = { 450.f, 500.f };	// 10/28 밸런스 : 380·420 → 450·500

public:
	// 거리 조건(부모) + 살아 있는 분신이 없을 때만
	virtual bool CanUsePattern(const AActor* Boss, const AActor* Target) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 이륙 노티파이 → 분신 생성 + 뒤로 도약
	UFUNCTION()
	void OnTakeoffEvent(FGameplayEventData Payload);
};
