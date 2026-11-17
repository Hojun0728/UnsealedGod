// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Base.h"
#include "GameplayAbility_Bound.generated.h"

// ============================================================================
// UGameplayAbility_Bound
// 보스의 속박(Subjugation)에 걸린 플레이어. (10/21) 넘어짐(UGameplayAbility_Knockdown)과 같은 구조.
//
// 흐름
//   1. 게임플레이 이벤트(Event.Player.Bound, Instigator = 보스)로 발동 (속박 패턴이 보냄)
//   2. 하던 스킬·회피·물약 취소 → 무적 (묶인 동안 미니언에게도 안 맞는다. 보스의 마무리 일격만 "막을 수 없는" 피해로 들어온다)
//   3. AM_Gideon_Bound (Emote_Master_Levitate_T3의 119~300프레임 = 이미 몸이 들린 구간) + 묶임 이펙트
//   4. 보스의 마무리 일격(Heavy)을 맞으면 넘어짐 어빌리티가 이 어빌리티를 취소하며 이어받는다.
//      (일격이 오지 않으면 mMaxBoundTime 뒤 스스로 풀린다 = 안전장치)
//
// 상태 태그 State.Bound(ActivationOwnedTags)가 붙어 있는 동안 APlayerCharacter::CanAct가 모든 입력을 막는다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Bound : public UGameplayAbility_Base
{
	GENERATED_BODY()

public:
	UGameplayAbility_Bound();

protected:
	// 묶인 몽타주. (11/4) 생성자에서 미리 불러온다. (처음 묶이는 순간 멈칫하지 않게)
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mBoundMontage;

	// 묶인 동안 몸에 붙는 이펙트 (세바로그 팩 P_SubjugateSwirls)
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mBoundParticle;

	// 붙인 이펙트 컴포넌트. 끝날 때 지운다.
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent>	mBoundParticleComponent;

	// 마무리 일격이 오지 않을 때 스스로 풀리는 시간(초). 정상이라면 약 1초 뒤 일격에 넘어짐으로 넘어간다.
	float	mMaxBoundTime = 4.f;

	// 이번 발동에서 무적 태그를 붙였는지 (붙이기 전에 끝나는 경우 떼지 않게)
	bool	mInvincibleAdded = false;

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
	// 안전장치 시간이 지남 → 스스로 풀린다
	UFUNCTION()
	void OnBoundTimeout();
};
