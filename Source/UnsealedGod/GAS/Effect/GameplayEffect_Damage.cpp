// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayEffect_Damage.h"
#include "../BaseAttributeSet.h"

UGameplayEffect_Damage::UGameplayEffect_Damage()
{
	// DurationPolicy : 이 효과가 얼마나 지속되는지.
	//   Instant     : 즉시 한 번 적용하고 끝. (BaseValue를 바꾼다)
	//   HasDuration : 정해진 시간 동안 지속. (쿨다운, 일시 버프)
	//   Infinite    : 직접 제거할 때까지 계속 지속. (마나 회복)
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Modifier : "어떤 Attribute를 어떻게 바꿀지"에 대한 정보 하나.
	FGameplayModifierInfo ModifierInfo;

	// AttributeSet이 가지고 있는 HP를 변경할 수 있게 한다.
	ModifierInfo.Attribute = UBaseAttributeSet::GetHPAttribute();

	// Additive : 현재 값에 더한다. 그래서 피해량은 음수(-10)로 넘겨준다.
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	// 피해량은 공격마다 다르기 때문에 고정값 대신 SetByCaller를 사용한다.
	// SetByCaller : "값은 나중에 적용하는 쪽(Caller)이 태그를 이름표 삼아 넣어준다"는 방식이다.
	FSetByCallerFloat DamageCaller;
	DamageCaller.DataTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage"));

	// SetByCaller를 ModifierInfo에 넣어준다.
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageCaller);

	Modifiers.Add(ModifierInfo);
}
