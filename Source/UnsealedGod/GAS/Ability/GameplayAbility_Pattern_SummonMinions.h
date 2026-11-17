// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_SummonMinions.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_SummonMinions
// 세바로그의 미니언 소환. 2페이즈부터 페이즈마다 한 번 (2페이즈 3마리, 3페이즈 5마리, 4페이즈 7마리).
// 페이즈에 들어가면 보스가 "소환 대기"를 켜고, 페이즈 시작 5초 뒤 우선순위 5로 반드시 이 패턴을 쓴다.
// (필살기와 같은 "대기 + 우선순위" 구조)
//
// 몽타주 AM_Sevarog_Summon (Cast : 왼손을 쫙 펼침)
//   손을 펼치는 순간 소환 노티파이(Event.Boss.Summon) → 레벨의 소환 지점(액터 태그 "MinionSpawn")에서 미니언 등장
// 소환 지점들을 돌아가며 쓰고(라운드 로빈), 같은 지점의 미니언끼리 겹치지 않게 조금씩 흩뜨린다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_SummonMinions : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_SummonMinions();

protected:
	// 소환할 미니언 클래스
	UPROPERTY()
	TSubclassOf<class AMinion>	mMinionClass;

	// (10/29) 소환하는 순간 "땅에서 솟아오르는" 소리 (보스 자리에서)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mSummonSound;

	// 소환 지점에서 미니언을 흩뜨리는 반지름(cm)
	float	mScatterRadius = 150.f;

	// 이번에 소환할 수 (시작할 때 보스에게서 꺼내 온다)
	int32	mSummonCount = 0;

public:
	// 보스에게 "소환 대기"가 있고 페이즈 시작 후 충분히 지났을 때만
	virtual bool CanUsePattern(const AActor* Boss, const AActor* Target) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 몽타주의 Event.Boss.Summon 노티파이 → 미니언 소환
	UFUNCTION()
	void OnSummonEvent(FGameplayEventData Payload);
};
