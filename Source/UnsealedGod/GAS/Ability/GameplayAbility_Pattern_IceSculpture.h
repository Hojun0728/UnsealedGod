// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_IceSculpture.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_IceSculpture
// 오로라의 얼음 조각 (모든 페이즈) = 얼어붙은 기디언에게 다가가 연속으로 찌르는 후속타. (10/28)
// 원작 감정표현 Emote_Ice_Sculpture(얼음 덩어리를 검으로 조각)의 의미를 살렸다 : 얼어붙은 기디언 = 조각할 얼음.
//
// 사용 조건 : 기디언이 얼어 있음(State.Frozen) + 10m 안. 우선순위 8 → 얼면 다음 패턴은 반드시 이것 (필살기 10 다음)
// 흐름
//   활성화 : 기디언 앞 mStrikeDistance까지 mApproachTime초 만에 미끄러지듯 다가가며 찌르기 동작 시작
//   → 도착 후 mStabInterval초마다 mStabCount번 : 아직 얼어 있고 가까우면 찌르기 피해(×0.4) + 얼음 파편
//   → 중간에 기디언이 연타로 깨고 나오면 그 자리에서 찌르기를 멈추고 후딜 mEscapeRecoveryTime초 = 연타를 잘한 보상(반격 틈)
//
// 찌를 때마다 판정 막대를 놓는 대신 코드가 일정 간격으로 피해를 준다. (11번 막대를 맞추는 대신 "얼어 있는 동안 찌른다"는 규칙)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_IceSculpture : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_IceSculpture();

protected:
	// ---------------------------------------------------------------- 수치 (10/28)
	float	mApproachTime = 0.3f;			// 다가가는 시간(초)
	float	mStabInterval = 0.3f;			// 찌르기 간격(초) = 찌르기 구간 3.3초 ÷ 약 11번
	int32	mStabCount = 11;				// 몽타주가 없을 때만 쓰는 기본 횟수 (보통은 GetStabDuration으로 계산)
	float	mStabDamageScale = 0.4f;
	float	mStabReach = 300.f;				// 이 거리 안에 있어야 찌르기가 맞는다 (다가간 뒤 기디언이 밀려나도 너그럽게)
	float	mEscapeRecoveryTime = 1.5f;		// 연타로 깨고 나왔을 때의 후딜

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mStabParticle;	// 찌를 때 튀는 얼음 파편

	// (10/29) 찌를 때 "쉿" (여러 개 중 랜덤). 얼음 포박이 생성자째로 물려받는다
	UPROPERTY()
	TArray<TObjectPtr<class USoundBase>>	mStabSounds;

	// ---------------------------------------------------------------- 실행 중
	bool	mEscaped = false;

public:
	// 거리 조건(부모) + 기디언이 얼어 있는지
	virtual bool CanUsePattern(const AActor* Boss, const AActor* Target) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 다가가기 끝 → 찌르기 반복 시작
	UFUNCTION()
	void OnApproachFinished();

	// 찌르기 한 번 (ActionNumber : 몇 번째인지, 0부터)
	UFUNCTION()
	void OnStab(int32 ActionNumber);

	// 찌르는 동작의 실제 길이(초). 찌르기 횟수를 이것으로 정한다. virtual : 얼음 포박은 Stab 섹션 길이로 덮어쓴다
	virtual float GetStabDuration()	const;

	// 대상이 얼어 있는지 (태그 확인)
	static bool IsTargetFrozen(const AActor* Target);
};
