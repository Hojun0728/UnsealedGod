// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_Exhausted.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_Exhausted
// 세바로그의 "지침(과부하)" (10/19). 기획의 그로기를 새 조건으로 되살린 것.
//
// 서사 : 3페이즈부터 커지고 빨라진 건 신이 무리하게 힘을 끌어올린 "과부하" 상태다.
//        그 대가로 일반 패턴을 4~6번 쓸 때마다 고개를 숙이고 지친다. (몬스터헌터 월드의 "지침" 참고)
// 게임 : 지친 동안(3페이즈 5초, 4페이즈 4초) 보스는 공격·이동을 하지 않는다 → 플레이어의 회복(물약) 또는 딜 시간.
//
// 동작 : Stun_Start → Stun_Loop(반복) → Stun_End. 세 애니메이션의 프레임 레이트가 모두 달라
//        한 몽타주에 넣을 수 없어서 즉석 몽타주로 이어 재생한다. (기디언 넘어짐과 같은 방식)
// 조건 : 보스가 "지침 대기"(ABossBase::IsExhaustPending)일 때만. 우선순위 8 (필살기 10보다 낮다)
//
// (11/4) 오로라도 쓴다 : 자식 UGameplayAbility_Pattern_AuroraExhausted가 생성자에서 동작·보이스·시간만 바꾼다.
//   세바로그 = 과부하가 쌓이면 / 오로라 = 필살기(얼음 지진)를 피하면 → 둘 다 "지침 대기"로 들어온다
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_Exhausted : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_Exhausted();

protected:
	// 페이즈별 지친 시간(초). 칸 번호 = 페이즈 - 1. (3페이즈 5초, 4페이즈 4초)
	TArray<float>	mExhaustTimes = { 5.f, 5.f, 5.f, 4.f };

	UPROPERTY()
	TObjectPtr<UAnimSequence>	mStartAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequence>	mLoopAnim;

	// 다시 자세를 잡는 동작. (11/4) 없으면(nullptr) 반복 동작을 mNoEndBlendOut초 동안 섞으며 서 있는 자세로 돌아간다
	UPROPERTY()
	TObjectPtr<UAnimSequence>	mEndAnim;

	float	mNoEndBlendOut = 0.4f;

	// 이번 지침에서 반복 동작을 유지할 시간 (= 전체 - 시작 - 끝)
	float	mLoopTime = 0.f;

	// ---------------------------------------------------------------- 보이스 (10/29, 팩 보이스. 즉석 몽타주라 노티파이를 못 붙여 코드로)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mStartVoice;	// 무너지는 순간 "크으…" (Sevarog_Effort_Frustration)

	UPROPERTY()
	TObjectPtr<class USoundBase>	mBreathVoice;	// 지쳐 있는 동안 헐떡임 (Sevarog_Effort_BreathingLowHealth)

	// 재생 중인 헐떡임 (일어날 때 줄이며 끈다)
	UPROPERTY()
	TObjectPtr<class UAudioComponent>	mBreathAudio;

	void StopBreath();

public:
	virtual bool CanUsePattern(const AActor* Boss, const AActor* Target) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 애니메이션 하나를 즉석 몽타주로 재생하고, 그 시간(초)을 돌려준다. (LoopCount : 반복 횟수, BlendOut : 블렌드 아웃 시간)
	float PlayAnim(UAnimSequence* Anim, int32 LoopCount, float BlendOut);

	// 다음 동작이 섞여 들어오는 시간(초). 다음 동작은 앞 동작이 끝나기 이만큼 전에 시작한다.
	float	mBlendTime = 0.15f;

	// 단계 : 시작 끝 → 반복 / 반복 끝 → 일어나기 / 일어나기 끝 → 종료 (WaitDelay 태스크가 부른다)
	UFUNCTION()
	void OnStartFinished();

	UFUNCTION()
	void OnLoopFinished();

	UFUNCTION()
	void OnEndFinished();

	// 보스가 죽었으면 더 진행하지 않고 끝낸다. 계속해도 되면 true.
	bool CheckAlive();
};
