// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayEffect_ManaRegen.h"
#include "../BaseAttributeSet.h"

UGameplayEffect_ManaRegen::UGameplayEffect_ManaRegen()
{
	// Infinite : 제거하기 전까지 계속 유지된다.
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Period : 이 시간(초)마다 Modifier를 한 번씩 "실행"한다.
	// Period가 있는 효과는 실행될 때마다 BaseValue를 바꾼다. (Instant를 주기적으로 반복하는 느낌)
	// FScalableFloat : 레벨에 따라 값이 바뀔 수 있는 float. 여기서는 고정값 1초로 쓴다.
	Period = FScalableFloat(1.f);

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UBaseAttributeSet::GetMPAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	// 회복량은 캐릭터마다 다르게 줄 수 있도록 SetByCaller를 사용한다.
	FSetByCallerFloat RegenCaller;
	RegenCaller.DataTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.MPRegen"));

	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(RegenCaller);

	Modifiers.Add(ModifierInfo);

	// MP가 최대치일 때도 계속 실행되지만,
	// UBaseAttributeSet::PreAttributeBaseChange에서 MPMax로 잘라주기 때문에 넘치지 않는다.
}
