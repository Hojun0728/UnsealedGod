// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_PlayerSkill.h"
#include "GameplayAbility_Potion.generated.h"

// ============================================================================
// UGameplayAbility_Potion
// 회복 물약 (1번 키, 10/19). 링이 커져서 몸을 위에서 아래로 훑는 동안 HP가 차오른다.
// 몽타주 : AM_Gideon_Potion (Recall 0~258프레임을 두 조각 : 링이 커지는 구간 3배 / 훑는 구간 2배, 총 약 3.5초)
//          링이 몸에 닿는 순간 Event.Skill.Fire
//
// 스킬 부모(UGameplayAbility_PlayerSkill)를 그대로 쓴다. "발동 순간 할 일"(OnFire) = 회복 시작.
//   - 개수는 첫 회복(HP 10)이 들어가는 순간 줄어든다 (링이 닿기 전에 끊으면 그대로)
//   - 회복은 OnFire부터 mHealInterval초마다 mHealPerTick씩 mHealTickCount번 (10 × 5 = 50)
//   - 이동 키·E를 누르거나 큰 공격에 넘어지면(어빌리티 취소) 그때까지 회복한 만큼만 남는다
//   - 마시는 동안 공격·점프·다른 스킬 불가 (State.Casting), 무적 없음
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Potion : public UGameplayAbility_PlayerSkill
{
	GENERATED_BODY()

public:
	UGameplayAbility_Potion();

protected:
	// 한 번에 회복하는 양, 횟수, 간격(초).
	// 간격 = 노티파이(2.32초) ~ 몽타주 끝(약 3.4초) 사이 약 1.06초 ÷ 5 ≈ 0.2 → 링이 지나가는 동안 고르게 10씩 (10/19)
	float	mHealPerTick = 10.f;
	int32	mHealTickCount = 5;
	float	mHealInterval = 0.2f;

	// 이번 시전에서 회복한 횟수
	int32	mHealTicksDone = 0;

	FTimerHandle	mHealTimer;

	// ---------------------------------------------------------------- 회복 이펙트 (10/21, 나이아가라 직접 제작)
	// 오라 : 회복하는 동안 몸에서 붉은 빛이 피어오른다. 끝나거나 끊기면 끈다.
	// 십자가 : HP가 한 번 찰 때마다(5번) 하나씩 떠올랐다 사라진다. → "10씩 차는" 것이 눈에 보인다.
	// (11/4) 생성자에서 미리 불러온다. (10/21에는 처음 쓸 때 경로로 불러왔다 → 세바로그전 첫 회복 순간 디스크에서 읽느라 멈칫)
	UPROPERTY()
	TObjectPtr<class UNiagaraSystem>	mAuraSystem;

	UPROPERTY()
	TObjectPtr<class UNiagaraSystem>	mCrossSystem;

	// 십자가가 생기는 높이 (캡슐 중심 기준, cm). 가슴 앞쯤
	float	mCrossHeight = 40.f;

	// 지금 켜져 있는 오라 (끝날 때 끈다)
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent>	mAuraComponent;

	// (10/29) 링이 몸을 훑으며 반짝이는 회복 소리. 링이 닿는 순간(OnFire) 튼다
	UPROPERTY()
	TObjectPtr<class USoundBase>	mHealSound;

	// 재생 중인 회복 소리 (끊겼을 때만 줄이며 끈다. 다 마셨으면 반짝임이 끝까지 울리게 둔다)
	UPROPERTY()
	TObjectPtr<class UAudioComponent>	mHealAudio;

public:
	// 물약이 남아 있어야 발동할 수 있다. (부모의 마나·쿨다운 확인에 더한다)
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 링이 몸에 닿는 순간 : 회복 시작
	virtual void OnFire(class APlayerCharacter* Player) override;

	// mHealInterval초마다 한 번 회복
	void HealTick();

	// 오라를 끈다 (회복이 끝나거나 끊길 때)
	void StopAura();
};
