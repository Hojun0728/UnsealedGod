// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Base.h"
#include "GameplayAbility_Knockdown.generated.h"

// ============================================================================
// UGameplayAbility_Knockdown
// 플레이어가 "큰 공격"(도약 착지·돌진 마무리·3연타 3타)에 맞았을 때 넘어지는 동작. (10/16)
//
// 흐름
//   1. 게임플레이 이벤트(Event.Player.Knockdown, Instigator = 공격자)로 발동 (APlayerCharacter::OnDamaged가 보냄)
//   2. 하던 스킬·회피 취소 → 몸을 공격자 반대쪽으로 돌림 → 무적
//   3. Knock_Fwd(앞으로 날아가는 자세) + 공격자 반대 방향으로 밀려남 (루트 모션 힘 태스크)
//   4. mAirTime 뒤 Stun_End(한쪽 무릎 → 일어섬)를 즉석 몽타주로 이어 재생
//   5. Stun_End가 끝나면 종료 → 무적·조작 불가 해제, 몸이 다시 카메라 방향
//   (10/19) 4~5 사이(무릎을 꿇은 뒤)에는 E로 넘어짐을 끊고 회피할 수 있다 = 기상 회피 (State.CanWakeDodge)
//           누워 있는 동안 보스의 1·2타는 무적으로 흘려보내고, 3타 직전에 E로 빠져나가는 타이밍 싸움
//
// 어빌리티로 만든 이유 : "넘어져 있는 동안"이라는 상태를 ActivationOwnedTags(State.Knockdown)로 표현하면
// 끝날 때 GAS가 태그를 자동으로 떼 준다. 회피·스킬과 같은 구조(몽타주 대기·딜레이 태스크)로 만들 수 있다.
//
// Knock_Fwd는 공중 자세로 끝나서 실제 착지가 없다 → 캐릭터를 진짜로 띄우지 않고(캡슐은 땅),
// 몸이 뜨는 것은 애니메이션이, 뒤로 밀리는 것은 루트 모션 힘이 담당한다. 정해진 시간 뒤 무릎 꿇기로 넘어간다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Knockdown : public UGameplayAbility_Base
{
	GENERATED_BODY()

public:
	UGameplayAbility_Knockdown();

protected:
	// 날아가는 몽타주 (Knock_Fwd). (11/4) 아래 동작 2개와 함께 생성자에서 미리 불러온다. (큰 피격 첫 순간에 멈칫하지 않게)
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mKnockMontage;

	// 일어나는 동작 (Stun_End). 즉석 몽타주로 재생한다. (Knock_Fwd 29.5fps / Stun_End 29.4fps라 한 몽타주에 못 넣음)
	UPROPERTY()
	TObjectPtr<UAnimSequence>	mStandUpAnim;

	// 앞·옆에서 맞았을 때 무릎 꿇는 동작 (Stun_Start, 10/19). 즉석 몽타주로 재생한다.
	UPROPERTY()
	TObjectPtr<UAnimSequence>	mFallAnim;

	// 넘어지는 시간(초). 이 시간 뒤 일어나기(Stun_End)로 넘어간다. 앞·옆의 Stun_Start도 이 시간에 맞춰 재생
	float	mAirTime = 0.6f;

	// 밀려나는 속도(cm/초). 0.6초 × 600 = 약 3.6m
	float	mPushSpeed = 600.f;

	// 앞·옆에서 맞았을 때 밀려나는 시간(초). 0.3 × 600 = 약 1.8m (등 뒤는 mAirTime 동안 3.6m)
	float	mFrontPushTime = 0.3f;

	// Stun_End 재생 속도 (애니메이션 에셋의 속도 스케일과 곱해진다)
	float	mStandUpPlayRate = 1.f;

	// Stun_End가 섞여 들어오는 시간(초). Stun_Start는 이만큼 더 길게 재생해서 사이에 서 있는 자세가 끼지 않게 한다.
	float	mStandUpBlendIn = 0.15f;

	// 일어나는 중인지. (날아가는 몽타주가 Stun_End에 밀려 끝나는 것은 "중단"이 아니므로 무시하기 위함)
	bool	mStandingUp = false;

	// 이번 발동에서 무적 태그를 붙였는지. (붙이기 전에 끝나는 경우 떼지 않게)
	bool	mInvincibleAdded = false;

	// 기상 회피 태그(State.CanWakeDodge)를 붙였는지 (10/19)
	bool	mWakeDodgeTagAdded = false;

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
	// 날아가는 시간이 끝났을 때 : Stun_End 재생
	UFUNCTION()
	void OnAirFinished();

	// 날아가는 몽타주가 끊겼을 때 (사망 등). 일어나는 중이 아니면 종료한다.
	UFUNCTION()
	void OnKnockMontageInterrupted();

	// Stun_End가 끝났을 때 : 종료
	UFUNCTION()
	void OnStandUpFinished();
};
