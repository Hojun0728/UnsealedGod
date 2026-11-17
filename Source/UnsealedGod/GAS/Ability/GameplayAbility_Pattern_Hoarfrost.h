// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_Hoarfrost.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_Hoarfrost
// 오로라의 서리 고리 (원작 Hoarfrost, 모든 페이즈). (10/28)
// 낮게 쓸어 베는 순간 오로라 중심 반지름 5m 원 안의 기디언이 피해를 받고 얼어붙는다. (UGameplayAbility_Frozen)
//
// 흐름
//   활성화 : 바닥에 원 예고(세바로그 도약과 같은 데칼)가 베는 순간까지 차오른다. 예고가 길게 보이도록 0.6배속.
//   → 베는 순간 노티파이(Event.Boss.Hoarfrost) : 1.0배속으로 돌아가고, 바닥 얼음·회오리 이펙트
//     원 안 + 땅에 서 있음 + 회피 무적 아님 → 피해(×1.0) + 얼어붙음 이벤트(최대 5초)
//   피하는 법 : 점프(공중이면 안 걸림) / E 무적 / 원 밖으로
// 안 걸렸으면 후딜(1.0 ± 0.3초)이 반격 틈이다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_Hoarfrost : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_Hoarfrost();

protected:
	// ---------------------------------------------------------------- 수치 (10/28 직접 결정)
	float	mRingRadius = 500.f;		// 원 반지름(cm). 기디언 캡슐 반지름만큼 더 너그럽게 판정한다
	float	mPhase2RingRadius = 700.f;	// 2페이즈(얼음 여제) 원 반지름 (10/28 밸런스)
	float	mCurrentRadius = 500.f;		// 이번에 쓰는 반지름 (활성화할 때 페이즈를 보고 정한다)
	float	mDamageScale = 1.f;			// 피해 = 공격력 × 배율
	float	mFreezeTime = 5.f;			// 최대 얼음 시간(초). 연타 16번이면 더 일찍 풀린다 (10/28 : 3 → 5초, 연타가 의미 있게)
	float	mHitTime = 11.f / 30.f;		// 베는 순간(몽타주 시간) = 노티파이 위치. 예고 데칼이 이 시간 동안 차오른다
	float	mAfterHitPlayRate = 1.f;	// 베고 난 뒤 재생 속도 (예고만 느리게)

	// ---------------------------------------------------------------- 이펙트·소리 (이름으로 고른 기본값 → 게임에서 보고 바꾼다)
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mGroundParticle;	// 바닥이 얼어붙음

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mSwirlParticle;		// 서리 회오리

	float	mParticleScale = 1.f;

	UPROPERTY()
	TObjectPtr<class USoundBase>	mCastSound;

	// ---------------------------------------------------------------- 실행 중
public:
	// 사용 거리 = 이번 페이즈의 원 반지름 안 (2페이즈는 7m라 더 멀리서도 쓴다)
	virtual bool CanUsePattern(const AActor* Boss, const AActor* Target) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// (11/4) EndAbility 덮어쓰기는 지웠다. 하던 일(예고 지우기)을 부모가 하게 되어서.

protected:
	// 베는 순간 노티파이 → 판정
	UFUNCTION()
	void OnHoarfrostEvent(FGameplayEventData Payload);

};
