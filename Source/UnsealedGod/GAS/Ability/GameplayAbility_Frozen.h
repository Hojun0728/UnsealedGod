// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Base.h"
#include "GameplayAbility_Frozen.generated.h"

// ============================================================================
// UGameplayAbility_Frozen
// 오로라의 얼음에 갇힌 플레이어. (10/28) 세바로그 속박(UGameplayAbility_Bound)과 같은 구조.
//
// 흐름
//   1. 게임플레이 이벤트(Event.Player.Frozen, EventMagnitude = 최대 얼음 시간)로 발동 (서리 고리·얼음 지진이 보냄)
//   2. 하던 스킬·회피·물약 취소 → 그 자세 그대로 멈춤(애니메이션 일시정지) + 몸 얼음 + 화면 성에
//   3. 이동 키를 "새로 누를" 때마다 기디언이 Event.Player.FrozenMash를 보낸다 → 여기서 센다
//      mMashRequired번 채우면 얼음이 깨지며 풀린다. 안 누르면 최대 시간 뒤 저절로 풀린다(안전장치).
//   4. 속박과 달리 무적이 아니다 → 얼어 있는 동안 오로라의 얼음 조각(찌르기)이 들어온다 = "연타할수록 덜 맞음"
//
// 상태 태그 State.Frozen(ActivationOwnedTags)이 붙어 있는 동안 APlayerCharacter::CanAct가 모든 입력을 막는다.
// 연타 감지는 입력을 받는 기디언이, 숫자 세기와 풀기는 이 어빌리티가 한다. (입력 쪽과 상태 쪽이 서로 몰라도 된다)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Frozen : public UGameplayAbility_Base
{
	GENERATED_BODY()

public:
	UGameplayAbility_Frozen();

protected:
	// ---------------------------------------------------------------- 수치 (10/28 직접 결정)
	// 풀려나는 데 필요한 연타 횟수
	int32	mMashRequired = 16;	// 10/28 : 8 → 16 → 24 → 16 (24는 얼어 있는 시간이 길어 다시 얼리는 "얼음 연쇄"가 생겼음)

	// 이벤트에 시간이 안 담겨 왔을 때의 최대 얼음 시간(초)
	float	mDefaultMaxTime = 5.f;

	// 얼음이 풀린 뒤 다시 얼지 않는 시간(초) (10/28 : 얼음 연쇄 방지)
	float	mImmuneTime = 3.f;

	// ---------------------------------------------------------------- 이펙트·소리 (오로라 팩 · 마법 효과음 팩)
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mBodyParticle;		// 몸을 덮는 얼음

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mScreenParticle;	// 화면 가장자리 성에 (카메라에 붙인다)

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mMashParticle;		// 연타할 때마다 튀는 얼음 조각

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mBreakParticle;		// 깨질 때

	// (10/29) 소리는 AI로 만든 효과음 (Sound/SFX)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mFreezeSound;		// 얼 때

	UPROPERTY()
	TArray<TObjectPtr<class USoundBase>>	mMashSounds;	// 연타 한 번마다 금 가는 "딱" (여러 개 중 랜덤 → 16번 반복이 기계적이지 않게)

	UPROPERTY()
	TObjectPtr<class USoundBase>	mBreakSound;		// 연타·시간 초과로 풀릴 때 (맑게 "쨍그랑")

	UPROPERTY()
	TObjectPtr<class USoundBase>	mShatterSound;		// 맞아서 깨질 때 (무겁게 "퍽 + 쨍그랑" = 1.5배 피해를 소리로)

	// ---------------------------------------------------------------- 실행 중
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent>	mBodyParticleComponent;

	UPROPERTY()
	TObjectPtr<UParticleSystemComponent>	mScreenParticleComponent;

	int32	mMashCount = 0;

	// 얼음 포박으로 얼어서 "안 깨짐" 태그를 붙였는지 (끝날 때 떼기 위해)
	bool	mNoShatterAdded = false;

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
	// 연타 한 번 (Event.Player.FrozenMash)
	UFUNCTION()
	void OnMash(FGameplayEventData Payload);

	// 맞아서 얼음이 깨짐 (Event.Player.FrozenShatter) → 바로 풀림
	UFUNCTION()
	void OnShatter(FGameplayEventData Payload);

	// 최대 시간이 지남 → 저절로 풀림
	UFUNCTION()
	void OnTimeout();

	// 얼음이 깨지며 풀린다. (연타 성공·시간 초과·맞아서 깨짐 공통) Sound : 그때 낼 소리
	void BreakFree(class USoundBase* Sound);
};
