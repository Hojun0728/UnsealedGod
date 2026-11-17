// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Base.h"
#include "GameplayAbility_PlayerSkill.generated.h"

// ============================================================================
// UGameplayAbility_PlayerSkill
// 플레이어 시전 스킬(Q·F·R)의 공통 부모 클래스다. (보스 패턴의 UGameplayAbility_BossPattern과 같은 생각)
//
// 세 스킬은 "시전 동작 → 정해진 순간에 발사 → 동작이 끝나면 종료"라는 흐름이 같고,
// 다른 것은 "발사하는 순간 무엇을 만드는가" 하나뿐이다.
//   Q 포털 운석 : 폭발하는 운석 투사체 / F 버든 : 도트 투사체 / R 운석 비 : 바닥 장판
// 그래서 흐름은 부모가 한 번만 쓰고, 자식은 virtual 함수 OnFire만 덮어쓴다.
// → 스킬을 하나 더 만들 때도 자식 클래스 하나 + OnFire만 쓰면 된다.
//
// 흐름
//   1. ActivateAbility : 몽타주 확인 → 비용·쿨다운 확정(부모) → "발사" 이벤트 대기 + 몽타주 재생
//   2. 몽타주의 Send Gameplay Event(Event.Skill.Fire) 노티파이 → OnFireEvent → OnFire (자식이 채운다)
//   3. 몽타주 끝 → EndAbility
//
// 시전 중 태그 (ActivationOwnedTags : 어빌리티가 켜져 있는 동안 자동으로 붙고 끝나면 자동으로 떨어진다)
//   State.Casting : 기본 공격·점프·다른 스킬을 막는다
//   (11/4) 회피(E)는 막지 않는다. Q·F는 SetDodgeCancelable()로 E에 끊기고, R은 자식이 State.NoDodge를 붙여 회피를 막는다
//   State.Rooted  : 이동을 막는다 (제자리 시전). 자식 생성자에서 SetAllowMoveWhileCasting(true)로 뺄 수 있다
// ============================================================================
UCLASS(Abstract)
class UNSEALEDGOD_API UGameplayAbility_PlayerSkill : public UGameplayAbility_Base
{
	GENERATED_BODY()

public:
	UGameplayAbility_PlayerSkill();

protected:
	// 시전 몽타주. 자식이 생성자에서 ConstructorHelpers로 넣는다.
	// (11/4 : 10/16~11/3에는 "처음 쓸 때 경로로" 불러왔다. 몽타주를 만든 뒤 에디터 재시작 없이 쓰려고.
	//  이제 에셋이 다 있으니 미리 불러 둔다 → 레벨마다 첫 시전 때 디스크를 읽느라 멈칫하던 것이 없어진다)
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mMontage;

	// 몽타주 재생 속도.
	float	mPlayRate = 1.f;

	// 이번 시전에서 발사했는지. 몽타주에 Fire 노티파이를 빠뜨렸을 때 로그로 알려주기 위함이다.
	bool	mFired = false;

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
	// true면 시전 중에도 이동할 수 있다. (State.Rooted 태그를 빼는 것)
	// 기본은 제자리 시전. 게임이 너무 어려우면 Q·F만 true로 바꿀 예정 (R은 항상 제자리 = 궁극기의 리스크)
	void SetAllowMoveWhileCasting(bool bAllow);

	// (11/4) E(회피)를 누르면 이 스킬을 끊고 순간이동한다. (Q·F 생성자에서 부른다)
	// 애셋 태그(이 어빌리티의 이름표)에 Ability.Skill.DodgeCancel을 붙인다.
	// 회피 어빌리티의 CancelAbilitiesWithTag가 이 이름표를 보고, 회피가 발동하는 순간 이 스킬을 취소한다.
	void SetDodgeCancelable();

	// 발사 순간 할 일. 자식이 덮어써서 투사체·장판을 만든다.
	// class APlayerCharacter* Player : 시전한 플레이어 (조준점·손 위치·공격력을 여기서 얻는다)
	// virtual ... { } : 부모는 아무것도 안 하는 기본 버전만 둔다.
	virtual void OnFire(class APlayerCharacter* Player)
	{
	}

	// 몽타주의 Event.Skill.Fire 노티파이를 받았을 때.
	UFUNCTION()
	void OnFireEvent(FGameplayEventData Payload);

	// (11/4) 몽타주가 끝났을 때(OnMontageFinished)는 부모(UGameplayAbility_Base)로 옮겼다. R이 덮어써서 루프를 잇는다.
};
