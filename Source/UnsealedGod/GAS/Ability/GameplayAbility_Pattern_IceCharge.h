// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Pattern_Charge.h"
#include "GameplayAbility_Pattern_IceCharge.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_IceCharge
// 오로라의 얼음 돌진 (원작 Glacial Charge, 모든 페이즈). (10/28)
// 세바로그 돌진(UGameplayAbility_Pattern_Charge)을 상속한다 → 방향 고정·사각형 예고·직선 질주·도착 처리는 부모 그대로.
// 오로라 버전이 더하는 것
//   - 질주하는 동안 정면 얼음 방패 : 보스에 State.FrontGuard 태그 → 정면에 맞은 투사체는 피해 없이 막힌다
//     (공략 = 옆으로 비켜서 옆구리를 쏘기)
//   - 얼음 이펙트 : 준비(모으기) / 질주 중 몸 앞 방패 / 멈출 때 방패가 부서짐
//   - 마무리 망치가 없다 : 목표 지점을 "뚫고 지나가" 몸통 판정(×1.5 + 넘어짐)이 곧 피해
//
// 몽타주 AM_Aurora_IceCharge (섹션 3개, 부모와 같은 방식)
//   Default(준비) : Ability_Q 0~16프레임 — 사각형 예고가 차오르고 끝에서 Event.Boss.ChargeStart
//   Run(달리기)   : Jog_Fwd_Ability_Q 무한 반복 — 몸통 판정(Melee Hit Window, spine_02)
//   Stop(멈춤)    : Ability_Q_Stop — 도착하면 부모가 이 섹션으로 넘긴다 (mSwingSectionName = "Stop")
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_IceCharge : public UGameplayAbility_Pattern_Charge
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_IceCharge();

protected:
	// ---------------------------------------------------------------- 이펙트 (이름만 보고 고른 기본값 → 게임에서 보고 생성자에서 바꾼다. 패턴은 블루프린트가 없어 디테일 칸이 없다)
	// 준비 동작 동안 몸에 붙는 기 모으기
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mWindupParticle;

	// 질주하는 동안 몸 앞에 붙는 방패 (여러 개를 겹쳐 붙일 수 있다 : 아래 조각 + 위 조각)
	UPROPERTY()
	TArray<TObjectPtr<UParticleSystem>>	mShieldParticles;

	// 멈출 때 방패가 부서지는 이펙트 (방패를 붙였던 자리에서)
	UPROPERTY()
	TArray<TObjectPtr<UParticleSystem>>	mShieldBreakParticles;

	// 방패를 붙일 위치 (보스 기준 : X = 앞, Z = 위, cm. 보스 크기 1.3배가 곱해진다)
	UPROPERTY()
	FVector	mShieldOffset = FVector(80.0, 0.0, 0.0);

	// (10/28) 방패 메시 : 위 이펙트(Wall_Base·Top)는 표면이 거의 안 보여서, 항상 보이는 얼음 가시 메시를 몸 앞에 붙인다.
	// 같은 자리(mShieldOffset)에 겹쳐 붙인다. 회전·크기는 게임에서 보고 맞춘다.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>>	mShieldMeshes;

	UPROPERTY()
	FRotator	mShieldMeshRotation = FRotator::ZeroRotator;

	UPROPERTY()
	float	mShieldMeshScale = 0.35f;	// 10/28 : 1.0은 오로라를 다 가릴 만큼 커서 0.35로 (내 판단)

	// 지금 붙어 있는 방패 메시 (끌 때 지운다)
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>>	mShieldMeshComponents;

	// ---------------------------------------------------------------- 소리 (10/29, 몸에 붙여 따라다닌다)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mWindupSound;	// 준비 : 차오르다 방패가 생기는 "팅" (연속 돌진의 다시 조준 때도)

	UPROPERTY()
	TObjectPtr<class USoundBase>	mDashSound;		// 질주 : 바람 가르는 소리 (달릴 때마다)

	// 소리를 몸에 붙여 튼다 (비어 있으면 아무것도 안 한다)
	void PlayAttachedSound(class USoundBase* Sound);

	// ---------------------------------------------------------------- 실행 중
	// 지금 붙어 있는 이펙트들 (멈출 때·중간에 끊길 때 꼭 끄기 위해 기억한다)
	UPROPERTY()
	TArray<TObjectPtr<UParticleSystemComponent>>	mActiveParticles;

	// 정면 방패를 켠 상태인지
	bool	mGuarding = false;

	// ---------------------------------------------------------------- 2페이즈 연속 돌진 (10/28)
	int32	mPhase1MinCharges = 1;		// 1페이즈 돌진 횟수 (10/28 밸런스 : 1 → 1~2)
	int32	mPhase1MaxCharges = 2;
	int32	mPhase2MinCharges = 2;		// 2페이즈 한 번에 돌진 횟수 (최소~최대 랜덤, 10/28 내 판단 : 2 또는 3)
	int32	mPhase2MaxCharges = 3;
	int32	mChargesThisTime = 1;		// 이번 패턴에서 달릴 횟수 (활성화할 때 정한다)
	float	mRechargeWindup = 0.4f;		// 두 번째 돌진 전 다시 조준·예고 시간(초)
	int32	mChargeCount = 0;			// 이번에 몇 번 달렸는지

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
	// 부모의 UFUNCTION을 덮어쓸 때는 UFUNCTION()을 다시 붙이지 않는다. (부모의 설정을 그대로 물려받는다)
	// 노티파이 → 부모가 등록한 다이나믹 델리게이트는 "함수 이름"으로 찾아 부르므로, 실제 객체(얼음 돌진)의 이 함수가 불린다.
	virtual void OnChargeStartEvent(FGameplayEventData Payload) override;

	virtual void StartSwing() override;

	// 다시 조준해서 한 번 더 달리기 준비 (예고 데칼 → mRechargeWindup초 뒤 OnRechargeReady)
	void BeginRecharge();

	UFUNCTION()
	void OnRechargeReady();

	// 방패 켜기 / 끄기 (끌 때 bBreak면 부서지는 이펙트)
	void SetGuard(bool bGuard, bool bBreak);

	// 붙어 있는 이펙트를 모두 끈다
	void ClearParticles();
};
