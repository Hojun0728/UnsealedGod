// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Base.h"
#include "GameplayAbility_TornSpace.generated.h"

// ============================================================================
// UGameplayAbility_TornSpace
// 기디언의 E 스킬 "토른 스페이스" : 이동 키 방향(없으면 뒤)으로 짧게 순간이동하는 회피.
// 마나 소모·쿨다운은 부모(UGameplayAbility_Base)가 처리하고, 여기서는 회피 동작만 만든다.
//
// 흐름
//   1. ActivateAbility : 비용·쿨다운 확정 → 도착 지점 계산 → 출발 이펙트 → 몽타주 재생
//   2. 몽타주의 "Event.Dodge.Teleport" 노티파이 → OnTeleportEvent : 도착 지점으로 이동 + 도착 이펙트
//   3. 몽타주 끝 → OnMontageFinished → EndAbility
// 어빌리티가 켜져 있는 동안 캐릭터에게 State.Dodging 태그가 붙는다(ActivationOwnedTags).
// → 캐릭터의 이동·공격 함수가 이 태그를 보고 입력을 무시한다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_TornSpace : public UGameplayAbility_Base
{
	GENERATED_BODY()

public:
	UGameplayAbility_TornSpace();

protected:
	// 회피 몽타주 (Torn_Space_v2). 생성자에서 경로로 불러온다.
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mMontage;

	// 몽타주 재생 속도. 0.93초짜리 동작을 1.3배로 → 약 0.7초.
	float	mPlayRate = 1.3f;

	// 순간이동 거리(cm).
	float	mDistance = 600.f;

	// 출발 지점·도착 지점에 터지는 포털 이펙트.
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mEnterParticle;

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mExitParticle;

	// (10/29) 빨려 들어감 → 튀어나옴 두 박자 소리. 누르는 순간 몸에 붙여 튼다 (도착 지점까지 따라간다)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mTeleportSound;

	// 이번 회피의 도착 지점. 발동할 때 계산해 두고, Teleport 이벤트 때 사용한다.
	// InstancedPerActor라서 어빌리티 객체가 캐릭터마다 따로 있으므로 변수에 상태를 저장할 수 있다.
	FVector		mDestination = FVector::ZeroVector;

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
	// 어빌리티 태스크의 델리게이트에 AddDynamic으로 등록하므로 UFUNCTION()이 필요하다.
	// (11/4) 몽타주가 끝났을 때(OnMontageFinished)는 부모(UGameplayAbility_Base)로 옮겼다.

	// 몽타주의 Teleport 노티파이가 보낸 게임플레이 이벤트를 받았을 때. 실제 순간이동.
	// FGameplayEventData Payload : 이벤트에 담겨 온 정보 (지금은 사용하지 않는다)
	UFUNCTION()
	void OnTeleportEvent(FGameplayEventData Payload);
};
