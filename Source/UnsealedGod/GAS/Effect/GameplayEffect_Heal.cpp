// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayEffect_Heal.h"
#include "../BaseAttributeSet.h"

UGameplayEffect_Heal::UGameplayEffect_Heal()
{
	// 즉시 한 번 적용 (BaseValue를 바꾼다)
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// HP에 더하기. 값은 SetByCaller(양수)
	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UBaseAttributeSet::GetHPAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat HealCaller;
	HealCaller.DataTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Heal"));

	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealCaller);

	Modifiers.Add(ModifierInfo);
}
