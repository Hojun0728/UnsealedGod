// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayEffect_Burn.h"
#include "../BaseAttributeSet.h"

UGameplayEffect_Burn::UGameplayEffect_Burn()
{
	// HasDuration : 정해진 시간 동안 지속된다.
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 지속 시간과 주기. FScalableFloat(값) : 고정값. (레벨별 표를 쓰면 레벨마다 다른 값도 가능)
	// 주기 0.5초, 붙는 순간에도 한 번 실행(bExecutePeriodicEffectOnApplication = true)
	//   → 0, 0.5, 1.0, ..., 3.5초 = 8번. 지속 시간을 4.0이 아니라 3.75로 둔 이유 :
	//     딱 4.0이면 "끝나는 순간"과 "9번째 실행"이 겹쳐서 8번인지 9번인지 애매해지기 때문이다.
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.75f));
	Period = FScalableFloat(0.5f);
	bExecutePeriodicEffectOnApplication = true;

	// Modifier : GE_Damage와 같다. HP에 더하기(Additive), 값은 SetByCaller(음수로 넘긴다).
	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UBaseAttributeSet::GetHPAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat DamageCaller;
	DamageCaller.DataTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage"));

	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageCaller);

	Modifiers.Add(ModifierInfo);
}
