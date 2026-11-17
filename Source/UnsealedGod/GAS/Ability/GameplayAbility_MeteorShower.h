// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_PlayerSkill.h"
#include "GameplayAbility_MeteorShower.generated.h"

// ============================================================================
// UGameplayAbility_MeteorShower
// 기디언 R "운석 비" (궁극기) : 공중에 떠올라 조준한 바닥에 3초 동안 운석을 쏟아붓는다.
// 몽타주 : AM_Gideon_MeteorShower (Blackhole_Start만), 쏘는 순간(끝 부분)에 Event.Skill.Fire
// 그 뒤 공중 유지 동작(BlackHole_Loop)은 코드가 이어서 한 번 재생한다. → 공중에 약 3초 (2배속)
//
// 루프를 몽타주에 같이 넣지 않은 이유 (10/16) :
//   Blackhole_Start는 29.739fps, BlackHole_Loop는 30fps라서 한 몽타주에 넣으면
//   엔진이 공통 프레임 레이트(100000fps)를 잡아 타임라인이 60만 프레임이 되고 에디터가 멈춘다.
//
// 궁극기의 리스크 (10/16 결정)
//   - 시전 중 무적 없음, 회피 불가 → 보스 패턴을 보고 "각을 재서" 써야 한다
//   - 10/19 : 이동 불가는 리스크가 너무 커서(테스트) 이동 가능으로. 회피·평타·스킬은 여전히 불가,
//     발사 전에 큰 공격에 맞으면(넘어짐이 어빌리티 취소) 운석 없이 끝난다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_MeteorShower : public UGameplayAbility_PlayerSkill
{
	GENERATED_BODY()

public:
	UGameplayAbility_MeteorShower();

protected:
	// 장판을 놓을 수 있는 최대 수평 거리(cm). 더 멀리 조준하면 이 거리까지만.
	float	mMaxDistance = 1500.f;

	// 한 번(0.3초마다) 피해 = 공격력 × 배율 (10 × 1.5 = 15) → 10번 = 150
	// 10/19 : 이동 가능으로 바뀌어 2.25 → 1.5로 되돌림 (반지름 7.2m는 유지)
	float	mPulseDamageScale = 1.5f;

	// 공중 유지 동작 (BlackHole_Loop). (11/4) 생성자에서 미리 불러온다.
	UPROPERTY()
	TObjectPtr<UAnimSequence>	mLoopAnim;

	// 이번 시전에서 루프를 시작했는지. (몽타주 "끝" 알림이 블렌드아웃·완료로 두 번 와도 한 번만 시작)
	bool	mLoopStarted = false;

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnFire(class APlayerCharacter* Player) override;

	// 시작 몽타주가 끝나면 바로 종료하지 않고 루프를 이어 붙인다.
	virtual void OnMontageFinished() override;

	// 루프 재생 시간이 끝났을 때. 어빌리티를 끝낸다.
	UFUNCTION()
	void OnLoopFinished();
};
