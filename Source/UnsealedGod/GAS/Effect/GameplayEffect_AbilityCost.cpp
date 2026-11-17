// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayEffect_AbilityCost.h"
#include "../BaseAttributeSet.h"

UGameplayEffect_AbilityCost::UGameplayEffect_AbilityCost()
{
	// 비용은 사용하는 순간 한 번 빠지고 끝나므로 Instant.
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// 마나
	FGameplayModifierInfo MPModifierInfo;
	MPModifierInfo.Attribute = UBaseAttributeSet::GetMPAttribute();
	MPModifierInfo.ModifierOp = EGameplayModOp::Additive;

	// 감소시킬 마나는 스킬마다 다르기 때문에 SetByCaller를 사용한다.
	FSetByCallerFloat MPCaller;
	MPCaller.DataTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.MPCost"));

	MPModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(MPCaller);

	Modifiers.Add(MPModifierInfo);

	// 체력
	FGameplayModifierInfo HPModifierInfo;
	HPModifierInfo.Attribute = UBaseAttributeSet::GetHPAttribute();
	HPModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat HPCaller;
	HPCaller.DataTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.HPCost"));

	HPModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(HPCaller);

	Modifiers.Add(HPModifierInfo);

	// 주의 : Modifier가 2개라서, 적용할 때 MPCost·HPCost 값을 "둘 다" 넣어줘야 한다.
	// 하나라도 빠지면 GAS가 "SetByCaller 값을 못 찾았다"는 에러 로그를 남긴다. (안 쓰는 쪽은 0을 넣는다)
}
