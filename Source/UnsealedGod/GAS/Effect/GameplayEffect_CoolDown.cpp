// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayEffect_CoolDown.h"

UGameplayEffect_CoolDown::UGameplayEffect_CoolDown()
{
	// Instant : 즉시 동작.
	// HasDuration : 지속시간이 존재.
	// Infinite : 무한 지속.
	// 쿨다운은 "정해진 시간 동안 태그를 붙여 두는 것"이라 HasDuration을 쓴다.
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 지속시간을 SetByCaller로 받는다. (스킬마다 쿨다운 시간이 다르므로)
	// 고정값을 쓰고 싶으면 : DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));
	FSetByCallerFloat CoolDownDuration;
	CoolDownDuration.DataTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.CoolDown"));

	DurationMagnitude = FGameplayEffectModifierMagnitude(CoolDownDuration);

	// 이 GE는 수치(Attribute)를 바꾸지 않는다. Modifier가 없고, 태그만 붙였다가 시간이 지나면 사라진다.
}
