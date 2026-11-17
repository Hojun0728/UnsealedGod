// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Base.h"
#include "../Effect/GameplayEffect_AbilityCost.h"
#include "../Effect/GameplayEffect_CoolDown.h"
#include "../BaseAttributeSet.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGameplayAbility_Base::UGameplayAbility_Base()
{
	// InstancedPerActor : 액터마다 어빌리티 객체를 하나 만들어 재사용한다.
	// 어빌리티 안에 상태(변수, 실행 중인 태스크)를 가질 수 있어서, 몽타주를 기다리는 스킬에 필요하다.
	// (NonInstanced는 CDO에서 실행되어 상태를 가질 수 없다)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

const FGameplayTagContainer* UGameplayAbility_Base::GetCooldownTags() const
{
	// 부모(UGameplayAbility)가 가진 쿨다운 태그 + 우리가 정한 mCoolDownTag를 합쳐서 돌려준다. (수업 코드와 동일)
	mCoolDownTagContainer.Reset();

	const FGameplayTagContainer* ParentTagContainer = Super::GetCooldownTags();

	if (ParentTagContainer)
		mCoolDownTagContainer.AppendTags(*ParentTagContainer);

	if (mCoolDownTag.IsValid())
		mCoolDownTagContainer.AddTag(mCoolDownTag);

	return &mCoolDownTagContainer;
}

// Ability 활성화시 호출되는 함수. Ability의 로직을 구현하는 곳
void UGameplayAbility_Base::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		mAbilityActive = false;

		// 어빌리티를 종료하고 싶다면 반드시 EndAbility를 호출해야 한다.
		// 그렇지 않으면 어빌리티가 끝나지 않고 계속 남아있게 된다.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// CommitAbility : 비용과 쿨다운을 "확정"한다. 내부에서 CheckCost·CheckCooldown을 한 번 더 하고,
	// 통과하면 ApplyCost(마나 차감)·ApplyCooldown(쿨다운 시작)을 호출한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		mAbilityActive = false;

		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	mAbilityActive = true;
}

void UGameplayAbility_Base::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	// (11/4) 엔진이 취소하는 순서 (UE 5.8 GameplayAbility.cpp · AbilityTask_PlayMontageAndWait.cpp)
	//   1. "취소된다" 알림 → 몽타주 태스크가 몽타주를 멈추고 OnInterrupted를 보낸다 (어빌리티는 아직 실행 중)
	//   2. EndAbility(…, bWasCancelled = true)
	// 1에서 OnMontageFinished가 EndAbility(취소 아님)를 먼저 불러 버리면, 2는 "이미 끝난 어빌리티"라 엔진이 무시한다.
	// → 끊긴 물약이 남은 회복을 다 주고, 발사 전에 넘어진 R이 운석을 쐈다. (11/4 코드 검토에서 발견)
	// 그래서 취소가 진행되는 동안 표시를 켜 두고, OnMontageFinished가 그 표시대로 "취소로" 끝낸다.
	mCancelling = true;

	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);

	mCancelling = false;
}

void UGameplayAbility_Base::PlayMontageAndEnd(UAnimMontage* Montage, float Rate)
{
	// 인자 : 소유 어빌리티, 태스크 이름(없음), 몽타주, 재생 속도
	UAbilityTask_PlayMontageAndWait* PlayMontage =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Rate);

	// 완료·블렌드아웃·중단·취소 중 어느 것이 와도 OnMontageFinished. (EndAbility가 두 번 불려도 엔진이 두 번째는 무시한다)
	PlayMontage->OnCompleted.AddDynamic(this, &UGameplayAbility_Base::OnMontageFinished);
	PlayMontage->OnBlendOut.AddDynamic(this, &UGameplayAbility_Base::OnMontageFinished);
	PlayMontage->OnInterrupted.AddDynamic(this, &UGameplayAbility_Base::OnMontageFinished);
	PlayMontage->OnCancelled.AddDynamic(this, &UGameplayAbility_Base::OnMontageFinished);

	// ReadyForActivation : 태스크를 실제로 시작한다. (만들기만 하고 이걸 안 부르면 동작하지 않는다)
	PlayMontage->ReadyForActivation();
}

void UGameplayAbility_Base::OnMontageFinished()
{
	// CurrentSpecHandle 등 : 부모 클래스가 들고 있는 "현재 실행 정보"
	// 마지막 인자 = 취소됐는지. 취소 중에 온 "끊겼다"면 true (CancelAbility 설명 참고)
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, mCancelling);
}

void UGameplayAbility_Base::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!ActorInfo || mCoolDownTime <= 0.f)
		return;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!ASC)
		return;

	// CoolDown을 적용하기 위해 UGameplayEffect_CoolDown Spec을 만들고, Cooldown 시간을 설정한다.
	FGameplayEffectSpecHandle	CoolDownSpec = MakeOutgoingGameplayEffectSpec(
		UGameplayEffect_CoolDown::StaticClass(), GetAbilityLevel());

	if (!CoolDownSpec.IsValid())
		return;

	CoolDownSpec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.CoolDown")), mCoolDownTime);

	// 이 GE가 붙어 있는 동안 캐릭터에게 줄 태그. (예 : Cooldown.TornSpace)
	if (mCoolDownTag.IsValid())
		CoolDownSpec.Data->DynamicGrantedTags.AddTag(mCoolDownTag);

	// 자기 자신에게 적용한다.
	ASC->ApplyGameplayEffectSpecToSelf(*CoolDownSpec.Data.Get());
}

bool UGameplayAbility_Base::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
		return false;

	// AbilitySystemComponent를 얻어온다.
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!ASC)
		return false;

	// 이 AbilitySystemComponent에 지정된 AttributeSet이 가지고 있는 현재 마나를 얻어와서
	// 소모량과 비교하여 사용이 가능한지를 판단한다. (수업 코드와 동일)
	float	CurrentMP = ASC->GetNumericAttribute(UBaseAttributeSet::GetMPAttribute());
	float	CurrentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHPAttribute());

	if (CurrentMP < mMPCost || CurrentHP < mHPCost)
		return false;

	return true;
}

void UGameplayAbility_Base::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// 부모의 ApplyCost는 "고정 비용 GE"를 쓰는 방식이라 부르지 않는다. (수업 코드와 동일)

	if (!ActorInfo)
		return;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!ASC)
		return;

	if (mMPCost <= 0.f && mHPCost <= 0.f)
		return;

	FGameplayEffectSpecHandle	CostSpec = MakeOutgoingGameplayEffectSpec(
		UGameplayEffect_AbilityCost::StaticClass(), GetAbilityLevel());

	if (!CostSpec.IsValid())
		return;

	// 수업 코드와 다른 점 : MP·HP 비용을 0이어도 "둘 다" 넣는다.
	// GE_AbilityCost는 Modifier가 MP·HP 2개라서, 하나라도 값이 없으면
	// GAS가 "SetByCaller 값을 못 찾았다"는 에러 로그를 남긴다. (어제 테스트 키에서 겪은 문제)
	// 음수로 넣어야 차감된다. (Additive)
	CostSpec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.MPCost")), -mMPCost);
	CostSpec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.HPCost")), -mHPCost);

	// 자기 자신에게 적용한다.
	ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data.Get());
}
