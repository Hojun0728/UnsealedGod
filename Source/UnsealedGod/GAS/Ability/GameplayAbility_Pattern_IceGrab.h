// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Pattern_IceSculpture.h"
#include "GameplayAbility_Pattern_IceGrab.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_IceGrab
// 오로라의 얼음 포박 (모든 페이즈) = 세바로그 속박과 같은 "잡기" 문법을 1스테이지에서 먼저 가르친다. (10/28)
// 얼음 조각(UGameplayAbility_Pattern_IceSculpture)을 상속한다 → 찌르기 반복·탈출하면 멈추기는 그대로 물려받는다.
//
// 몽타주 AM_Aurora_IceGrab : Emote_Ice_Sculpture 0~4.8초. 섹션 Default(0~1.39초, 손으로 얼음 보내기) → 없음 / Stab(1.39초~, 찌르기)
//   26프레임(0.87초)에 Event.Boss.IceGrab = 손에서 얼음이 나가는 순간
// 흐름 (세바로그 속박과 같은 단계)
//   ① 시전 : 기디언 발밑 원(1.5m)이 따라다니며 0.87초 동안 차오른다
//   ② 판정 : 원이 다 찬 순간 앞뒤 0.07초 안에 한 번이라도 회피 무적이면 실패 → 시전 동작 끝 + 후딜 1초 (반격 틈)
//   ③ 성공 : 기디언 얼어붙음 → 오로라가 얼음 파편과 함께 기디언 정면 또는 등 뒤(랜덤) 1.5m로 순간이동
//   ④ 찌르기 : Stab 섹션 + 부모의 찌르기 반복. 연타로 깨고 나오면 멈춤
// 페이즈 배속을 받지 않는다 (세바로그 속박과 같은 이유 : 예고가 짧아지면 E 타이밍을 배울 수 없다)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_IceGrab : public UGameplayAbility_Pattern_IceSculpture
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_IceGrab();

protected:
	// ---------------------------------------------------------------- 수치 (10/28)
	float	mCastTime = 26.f / 30.f;		// 손에서 얼음이 나가는 순간(몽타주 시간) = 원이 다 차는 순간
	float	mWarningRadius = 150.f;			// 기디언 발밑 예고 원
	float	mJudgeWindow = 0.14f;			// 판정 구간 (세바로그 속박과 같다)
	float	mAppearDistance = 150.f;		// 기디언에게서 얼마나 떨어져 나타날지
	float	mGrabFreezeTime = 5.f;			// 최대 얼음 시간
	float	mFailRecoveryTime = 1.f;		// 피했을 때 후딜 = 반격 틈

	FName	mStabSectionName = TEXT("Stab");

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mTeleportParticle;	// 사라지는 자리·나타나는 자리의 얼음 파편

	// (10/29) 예고 소리 : 원이 차오르다 0.87초(= 판정 순간)에 "팍" 터지도록 만든 소리. 원이 생길 때 기디언 몸에 붙여 튼다
	UPROPERTY()
	TObjectPtr<class USoundBase>	mChargeSound;

	UPROPERTY()
	TObjectPtr<class USoundBase>	mTeleportSound;		// 잡은 뒤 순간이동 "슉-팅"

	UPROPERTY()
	TObjectPtr<class USoundBase>	mTauntVoice;		// 잡은 뒤 도발 대사 (Aurora_Emote_Taunt)

	// 재생 중인 예고 소리 (판정 전에 패턴이 끊기면 끈다)
	UPROPERTY()
	TObjectPtr<class UAudioComponent>	mChargeAudio;

	// ---------------------------------------------------------------- 실행 중
	bool	mResolved = false;

	// 판정용 : 기디언이 마지막으로 무적이었던 시각 (세바로그 속박과 같은 방식)
	float	mLastInvincibleTime = -100.f;
	TWeakObjectPtr<UAbilitySystemComponent>	mTargetASC;
	FDelegateHandle	mInvincibleTagHandle;

public:
	// 부모(얼음 조각)의 "얼어 있을 때만" 조건이 아니라 거리(0~6m)만 본다
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
	UFUNCTION()
	void OnGrabCastEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnJudgeLate();

	bool IsDodgingNow()	const;
	void OnTargetInvincibleChanged(const FGameplayTag Tag, int32 NewCount);
	void StopWatchingInvincible();

	// 찌르기 길이 = Stab 섹션 길이 (손으로 얼음 보내는 Default 섹션은 빼고)
	virtual float GetStabDuration()	const override;

	// 판정 결과 처리 (피했으면 true)
	void ResolveGrab(bool bDodged);

};
