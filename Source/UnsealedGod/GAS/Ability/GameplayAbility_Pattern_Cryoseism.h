// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_Cryoseism.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_Cryoseism
// 오로라의 얼음 지진 (원작 궁극기 Cryoseism) = HP 50% 페이즈 전환 필살기. (10/28)
// 세바로그 필살기와 같은 규칙 : 체력 기준을 넘으면 "필살기 대기" → 우선순위 10이라 지금 패턴이 끝나면 반드시 나온다.
//
// 몽타주 AM_Aurora_Cryoseism : Ability_R 한 조각 (회전하며 떠오름), 22프레임(공중 정점)에 Event.Boss.Cryoseism
//   (10/28 수정 : 처음엔 1프레임 자세 반복 + 얼음 고치로 기를 모았지만 어색해서, Ability_R 자체를 0.37배속으로 늘림)
//
// 흐름
//   활성화 : 그 자리에서 0.37배속으로 천천히 회전하며 떠오르고, 발밑 반지름 10m 원 예고가 2초 동안 차오른다
//   → 폭발 노티파이(공중 정점) : 1.0배속으로 되돌림 + 큰 얼음 파편·폭발·화면 흔들림·저음 쾅 + 2페이즈 진입(= 오로라는 이 순간 얼음 여제로 변신)
//     원 안 + 회피 무적 아님 → 피해 ×1.5 + 얼어붙음 5초. 점프로는 못 피한다 (지진)
//   → 몽타주 끝 → 맞혔으면 바로 다음 패턴 / (11/4) 피했으면 지침(오로라 지침 패턴, 4초) = 회복 구간
// 공략 : 원 밖으로 달리기 (세바로그 = 기둥 뒤에 숨기 → 두 필살기의 답이 다르다)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_Cryoseism : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_Cryoseism();

protected:
	// ---------------------------------------------------------------- 수치 (10/28 직접 결정)
	float	mRadius = 1000.f;			// 원 반지름 10m
	float	mWindupTime = 2.f;			// 폭발까지의 예고(초, 실제 시간). 몽타주 재생 속도를 이 값에 맞춘다
	float	mBlastTime = 22.f / 30.f;	// 몽타주 시작 → 폭발 노티파이(몽타주 시간, 1.0배속 기준)
	float	mDamageScale = 1.5f;
	float	mFreezeTime = 5.f;

	// ---------------------------------------------------------------- 이펙트 (오로라 팩 Ultimate·Leap 폴더. 게임에서 보고 바꾼다)
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mWarmupParticle;	// 기 모으기 (몸에 붙음)

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mExplodeParticle;	// 폭발

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mGroundParticle;	// 바닥이 얼어붙음

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mShatterParticle;	// 폭발 순간의 큰 얼음 파편 (변신을 가린다)

	float	mShatterScale = 2.5f;

	// ---------------------------------------------------------------- 소리 (10/29)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mChargeSound;		// 기 모으기 (몸에 붙임)

	float	mChargeFadeTime = 0.15f;	// 폭발 순간 기 모으기 소리를 줄이며 끄는 시간 (세바로그 필살기와 같다)

	UPROPERTY()
	TObjectPtr<class UAudioComponent>	mChargeAudio;	// 재생 중인 기 모으기 소리

	// ---------------------------------------------------------------- 실행 중
	UPROPERTY()
	TArray<TObjectPtr<UParticleSystemComponent>>	mActiveParticles;

	// 끝나면 들어갈 페이즈 (필살기 대기에서 꺼낸 값), 폭발이 일어났는지
	int32	mTargetPhase = 0;
	bool	mBlasted = false;

	// (11/4) 폭발이 기디언을 맞혔는지. 못 맞혔으면 끝날 때 지침 대기를 세운다
	bool	mHitTarget = false;

public:
	// 거리 조건 대신 "필살기 대기"만 확인한다. (세바로그 필살기와 같다)
	virtual bool CanUsePattern(const AActor* Boss, const AActor* Target) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 폭발 노티파이 → 판정·연출·페이즈 진입
	UFUNCTION()
	void OnBlastEvent(FGameplayEventData Payload);

	void ClearParticles();
};
