// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../../GameInfo.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbility_Base.generated.h"

// ============================================================================
// UGameplayAbility_Base
// 이 프로젝트의 모든 스킬(GameplayAbility)의 부모 클래스다. (수업 코드를 거의 그대로 옮김)
// 스킬마다 반복되는 "마나·체력 비용"과 "쿨다운"을 여기서 공통으로 처리한다.
// 자식 클래스는 생성자에서 mMPCost, mCoolDownTime, mCoolDownTag만 정하고
// ActivateAbility에서 스킬 고유의 동작만 만들면 된다.
//
// GAS가 어빌리티를 발동하는 순서
//   TryActivateAbility → CanActivateAbility (여기서 CheckCost, CheckCooldown)
//   → ActivateAbility → CommitAbility (여기서 ApplyCost, ApplyCooldown) → ... → EndAbility
// 아래 virtual 함수들은 GAS가 이 순서 중간에 불러주는 함수를 덮어써서 우리 방식으로 바꾼 것이다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Base : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGameplayAbility_Base();

protected:
	// 쿨다운 시간(초). 0이면 쿨다운 없음.
	float							mCoolDownTime = 0.f;

	// 사용할 때 소모하는 마나·체력.
	float							mMPCost = 0.f;
	float							mHPCost = 0.f;

	// 쿨다운 동안 캐릭터에게 붙는 태그. (예 : Cooldown.TornSpace)
	// 스킬마다 다른 태그를 써야 "E는 쿨다운인데 Q는 사용 가능"이 된다.
	FGameplayTag					mCoolDownTag;

	// mutable은 const 함수 안에서도 이 변수만은 수정할 수 있게 허용한다. (수업 코드와 동일)
	// GetCooldownTags가 const 함수인데, 그 안에서 이 컨테이너를 채워서 주소를 돌려줘야 하기 때문이다.
	mutable FGameplayTagContainer	mCoolDownTagContainer;

	// 현재 Ability가 사용할 수 있는 Ability인지 판단하는 변수.
	// 자식 클래스는 Super::ActivateAbility 뒤에 이 값을 보고 계속할지 결정한다.
	bool		mAbilityActive = true;

	// (11/4) 지금 취소(CancelAbility) 중인지. OnMontageFinished가 이 값을 보고 "취소로 끝남"을 정확히 알린다. (CancelAbility 설명 참고)
	bool		mCancelling = false;

protected:
	// (11/4) 몽타주를 재생하고, 완료·블렌드아웃·중단·취소 중 무엇이 오든 OnMontageFinished를 부르는 태스크를 시작한다.
	// 스킬(Q·F·R·물약)·회피·보스 패턴이 똑같이 4줄씩 연결하던 것을 한 곳으로 모았다.
	void PlayMontageAndEnd(UAnimMontage* Montage, float Rate);

	// 몽타주가 끝났거나 끊겼을 때. 기본 : 어빌리티를 끝낸다. (취소 중이었으면 "취소로" 끝낸다)
	// virtual : 몽타주 뒤에 동작을 더 이어 붙여야 하는 스킬(R 운석 비의 공중 루프)이 덮어쓴다.
	// (UFUNCTION도 virtual로 만들 수 있다. 델리게이트가 불러도 실제 객체의 버전이 실행된다)
	UFUNCTION()
	virtual void OnMontageFinished();

public:
	// (10/21) HUD 스킬 칸이 읽는 값. 클래스 기본 객체(CDO)에서 읽기만 한다.
	float GetMPCost()	const
	{
		return mMPCost;
	}

	const FGameplayTag& GetCoolDownTag()	const
	{
		return mCoolDownTag;
	}

	// 이 어빌리티의 쿨다운 태그 목록. GAS의 CheckCooldown이 이 태그가 캐릭터에게 붙어 있는지 확인한다.
	// 반환형 const FGameplayTagContainer* : "읽기만 가능한" 컨테이너의 주소를 돌려준다.
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	// 어빌리티 발동 시 호출되는 함수. 비용·쿨다운을 확정(CommitAbility)한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// (11/4) 취소(넘어짐·속박·얼음·E로 끊기 등)가 진행되는 동안 mCancelling을 켠다.
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateCancelAbility) override;

	// 쿨다운 GE를 자기 자신에게 적용한다.
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 마나·체력이 충분한지 확인한다. false면 어빌리티가 발동되지 않는다.
	// OUT : 이 인자로 값을 "돌려받는다"는 표시(실제 기능은 없고 읽는 사람을 위한 표시).
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 마나·체력을 실제로 차감한다.
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;
};
